#include <blaze/mysql_pool.h>
#include <blaze/app.h>
#include <boost/asio/use_awaitable.hpp>
#include <regex>

namespace blaze {

MySqlPool::MySqlPool(boost::asio::io_context& ctx, std::string url, int size)
    : ctx_(ctx), url_(std::move(url)), size_(size) {
    parse_url();
}

MySqlPool::MySqlPool(App& app, std::string url, int size)
    : MySqlPool(app.engine(), std::move(url), size) {}

void MySqlPool::parse_url() {
    std::regex url_regex("mysql://([^:]+):([^@]+)@([^:/]+)(?::([0-9]+))?/(.+)");
    std::smatch matches;
    if (std::regex_search(url_, matches, url_regex)) {
        config_.user = matches[1].str();
        config_.pass = matches[2].str();
        config_.host = matches[3].str();
        config_.port = matches[4].matched ? (unsigned int)std::stoi(matches[4].str()) : 3306;
        config_.db = matches[5].str();
    }
}

void MySqlPool::connect() {
    auto self = shared_from_this();
    boost::asio::co_spawn(ctx_, [this, self]() -> boost::asio::awaitable<void> {
        try {
            co_await start();
        } catch (...) {}
    }, boost::asio::detached);
}

boost::asio::awaitable<void> MySqlPool::start() {
    for (int i = 0; i < size_; ++i) {
        auto conn = std::make_unique<MySqlConnection>(ctx_);
        try {
            co_await conn->connect(config_.host, config_.user, config_.pass, config_.db, config_.port);
        } catch (...) {}
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            available_.push(conn.get());
            pool_.push_back(std::move(conn));

            if (!waiters_.empty()) {
                auto timer = waiters_.front();
                waiters_.pop();
                timer->cancel();
            }
        }
    }
    co_return;
}

boost::asio::awaitable<MySqlConnection*> MySqlPool::acquire() {
    auto start_time = std::chrono::steady_clock::now();
    while (true) {
        std::shared_ptr<boost::asio::steady_timer> timer;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!available_.empty()) {
                auto* conn = available_.front();
                available_.pop();
                co_return conn;
            }
            
            if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(5)) {
                throw std::runtime_error("Timeout acquiring MySQL connection");
            }

            timer = std::make_shared<boost::asio::steady_timer>(ctx_, std::chrono::seconds(5));
            waiters_.push(timer);
        }
        try {
            co_await timer->async_wait(boost::asio::use_awaitable);
        } catch (...) {}
    }
}

void MySqlPool::release(MySqlConnection* conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(mutex_);
    available_.push(conn);
    if (!waiters_.empty()) {
        auto timer = waiters_.front();
        waiters_.pop();
        timer->cancel();
    }
}

boost::asio::awaitable<Json> MySqlPool::query(const std::string& sql, const std::vector<std::string>& params) {
    if (!breaker_.allow_request()) {
        throw std::runtime_error("MySQL Circuit Open: Too many recent failures");
    }

    auto* conn = co_await acquire();

    for (int attempt = 1; attempt <= 2; ++attempt) {
        try {
            if (!conn->is_open()) {
                co_await conn->connect(config_.host, config_.user, config_.pass, config_.db, config_.port);
            }

            auto res = co_await conn->query(sql, params);

            release(conn);
            breaker_.record_success();
            co_return Json(std::make_shared<MySqlResult>(std::move(res)));

        } catch (const std::exception& e) {
            conn->force_close();
            if (attempt == 2) {
                release(conn);
                breaker_.record_failure();
                throw;
            }
        }
    }

    release(conn);
    throw std::runtime_error("MySQL Query Failed");
}

} // namespace blaze