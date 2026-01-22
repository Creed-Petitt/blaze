#include <blaze/pg_pool.h>
#include <blaze/app.h>
#include <boost/asio/use_awaitable.hpp>

namespace blaze {

    PgPool::PgPool(boost::asio::io_context& ctx, std::string conn_str, int size)
        : ctx_(ctx), conn_str_(std::move(conn_str)), size_(size) {}

    PgPool::PgPool(App& app, std::string conn_str, const int size)
        : PgPool(app.engine(), std::move(conn_str), size) {}

    void PgPool::connect() {
        auto self = shared_from_this();
        boost::asio::co_spawn(ctx_, [this, self]() -> boost::asio::awaitable<void> {
            try {
                co_await start();
            } catch (...) {}
        }, boost::asio::detached);
    }

    boost::asio::awaitable<void> PgPool::start() {
        for (int i = 0; i < size_; ++i) {
            auto conn = std::make_unique<PgConnection>(ctx_);
            try {
                co_await conn->connect(conn_str_);
            } catch (...) {}
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                available_.push(conn.get());
                pool_.push_back(std::move(conn));
                
                if (!waiters_.empty()) {
                    auto timer = waiters_.front();
                    waiters_.pop();
                    timer->cancel(); // Wake up the waiter
                }
            }
        }
        co_return;
    }

    boost::asio::awaitable<PgConnection*> PgPool::acquire() {
        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            std::shared_ptr<boost::asio::steady_timer> timer;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!available_.empty()) {
                    PgConnection* conn = available_.front();
                    available_.pop();
                    co_return conn;
                }

                if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(5)) {
                    throw std::runtime_error("Timeout acquiring Postgres connection");
                }

                timer = std::make_shared<boost::asio::steady_timer>(ctx_, std::chrono::seconds(5));
                waiters_.push(timer);
            }
            try {
                co_await timer->async_wait(boost::asio::use_awaitable);
            } catch (...) {}
        }
    }

    void PgPool::release(PgConnection* conn) {
        if (!conn) return;
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(conn);
        if (!waiters_.empty()) {
            auto timer = waiters_.front();
            waiters_.pop();
            timer->cancel();
        }
    }

    boost::asio::awaitable<DbResult> PgPool::query(const std::string& sql, const std::vector<std::string>& params) {
        if (!breaker_.allow_request()) {
            throw std::runtime_error("Postgres Circuit Open: Too many recent failures");
        }

        PgConnection* conn = co_await acquire();

        for (int attempt = 1; attempt <= 2; ++attempt) {
            try {
                if (!conn->is_open()) {
                    co_await conn->connect(conn_str_);
                }

                PgResult res = co_await conn->query(sql, params);

                release(conn);
                breaker_.record_success();
                co_return DbResult(std::make_shared<PgResult>(std::move(res)));

            } catch (const std::exception& e) {
                conn->force_close();

                // If this was the last attempt, fail and trip the breaker
                if (attempt == 2) {
                    release(conn);
                    breaker_.record_failure();
                    throw;
                }
            }
        }
        
        // Should be unreachable
        release(conn);
        throw std::runtime_error("Postgres Query Failed");
    }

} // namespace blaze
