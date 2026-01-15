#include <blaze/pg_pool.h>
#include <blaze/app.h>
#include <boost/asio/use_awaitable.hpp>

namespace blaze {

    PgPool::PgPool(boost::asio::io_context& ctx, std::string conn_str, int size)
        : ctx_(ctx), conn_str_(std::move(conn_str)), size_(size) {}

    // Delegating constructor
    PgPool::PgPool(App& app, std::string conn_str, const int size)
        : PgPool(app.engine(), std::move(conn_str), size) {}

    void PgPool::connect() {
        boost::asio::co_spawn(ctx_, [this]() -> boost::asio::awaitable<void> {
            try {
                co_await start();
            } catch (const std::exception& e) {
                // Use logger in production
                fprintf(stderr, "[PgPool] Connection Error: %s\n", e.what());
            }
        }, boost::asio::detached);
    }

    boost::asio::awaitable<void> PgPool::start() {
        for (int i = 0; i < size_; ++i) {
            auto conn = std::make_unique<PgConnection>(ctx_);
            co_await conn->connect(conn_str_);
            available_.push(conn.get());
            pool_.push_back(std::move(conn));
        }
        co_return;
    }

    boost::asio::awaitable<PgConnection*> PgPool::acquire() {
        if (!available_.empty()) {
            PgConnection* conn = available_.front();
            available_.pop();
            co_return conn;
        }

        // Pool exhausted, must wait
        auto timer = std::make_shared<boost::asio::steady_timer>(
            ctx_, std::chrono::steady_clock::time_point::max()
        );
        waiters_.push(timer);

        try {
            co_await timer->async_wait(boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            if (e.code() != boost::asio::error::operation_aborted) {
                throw;
            }
        }

        // Woken up by release(), try again
        co_return co_await acquire();
    }

    void PgPool::release(PgConnection* conn) {
        if (!waiters_.empty()) {
            auto timer = waiters_.front();
            waiters_.pop();
            timer->cancel(); // Wake up the next waiter
        } else {
            available_.push(conn);
        }
    }

    boost::asio::awaitable<PgResult> PgPool::query(const std::string& sql) {
        PgConnection* conn = co_await acquire();
        try {
            PgResult res = co_await conn->query(sql);
            release(conn);
            co_return res;
        } catch (...) {
            release(conn);
            throw;
        }
    }

} // namespace blaze