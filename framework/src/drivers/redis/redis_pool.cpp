#define BOOST_REDIS_SEPARATE_COMPILATION
#include <blaze/redis.h>
#include <blaze/app.h>
#include <boost/redis/src.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <tuple>
#include <queue>
#include <mutex>

namespace blaze {

struct Redis::Impl {
    boost::asio::io_context& ioc;
    std::string host;
    std::string port;
    int pool_size;
    
    std::vector<std::unique_ptr<boost::redis::connection>> pool;
    std::queue<boost::redis::connection*> available;
    std::queue<std::shared_ptr<boost::asio::steady_timer>> waiters; // Queue for waiting coroutines
    std::mutex mtx;

    Impl(boost::asio::io_context& ctx, std::string h, std::string p, int sz)
        : ioc(ctx), host(std::move(h)), port(std::move(p)), pool_size(sz) {}
};

// RAII Guard to ensure connection is returned to pool
class RedisGuard {
    Redis& parent_;
    boost::redis::connection* conn_;
public:
    RedisGuard(Redis& p, boost::redis::connection* c) : parent_(p), conn_(c) {}
    ~RedisGuard() { parent_.release(conn_); }
    boost::redis::connection* get() { return conn_; }
};

Redis::Redis(App& app, std::string host, std::string port, int pool_size)
    : impl_(std::make_unique<Impl>(app.engine(), std::move(host), std::move(port), pool_size)) {}

Redis::~Redis() = default;

void Redis::connect() {
    for (int i = 0; i < impl_->pool_size; ++i) {
        auto conn = std::make_unique<boost::redis::connection>(boost::asio::make_strand(impl_->ioc));
        
        boost::asio::co_spawn(conn->get_executor(), [this, c = conn.get()]() -> boost::asio::awaitable<void> {
            boost::redis::config cfg;
            cfg.addr.host = impl_->host;
            cfg.addr.port = impl_->port;
            try {
                co_await c->async_run(cfg, {}, boost::asio::use_awaitable);
            } catch (...) {}
        }, boost::asio::detached);

        impl_->available.push(conn.get());
        impl_->pool.push_back(std::move(conn));
    }
}

boost::asio::awaitable<boost::redis::connection*> Redis::acquire() {
    while (true) {
        std::shared_ptr<boost::asio::steady_timer> timer;
        {
            std::lock_guard<std::mutex> lock(impl_->mtx);
            if (!impl_->available.empty()) {
                auto* conn = impl_->available.front();
                impl_->available.pop();
                co_return conn;
            }

            // Pool is empty, add to wait list
            timer = std::make_shared<boost::asio::steady_timer>(
                impl_->ioc, std::chrono::steady_clock::time_point::max()
            );
            impl_->waiters.push(timer);
        }

        try {
            co_await timer->async_wait(boost::asio::use_awaitable);
        } catch (const boost::system::system_error& e) {
            if (e.code() != boost::asio::error::operation_aborted) {
                throw; // Real error
            }
            // Operation aborted means we were woken up!
            // Loop back to try acquiring again
        }
    }
}

void Redis::release(boost::redis::connection* conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(impl_->mtx);
    impl_->available.push(conn);
}

boost::asio::awaitable<std::string> Redis::get(std::string_view key) {
    RedisGuard guard(*this, co_await acquire());
    boost::redis::request req;
    req.push("GET", key);
    boost::redis::response<std::string> res;
    co_await guard.get()->async_exec(req, res, boost::asio::use_awaitable);
    co_return std::get<0>(res).value();
}

boost::asio::awaitable<void> Redis::set(std::string_view key, std::string_view value, int expire_seconds) {
    RedisGuard guard(*this, co_await acquire());
    boost::redis::request req;
    if (expire_seconds > 0) {
        req.push("SET", key, value, "EX", std::to_string(expire_seconds));
    } else {
        req.push("SET", key, value);
    }
    co_await guard.get()->async_exec(req, boost::redis::ignore, boost::asio::use_awaitable);
}

boost::asio::awaitable<void> Redis::del(std::string_view key) {
    RedisGuard guard(*this, co_await acquire());
    boost::redis::request req;
    req.push("DEL", key);
    co_await guard.get()->async_exec(req, boost::redis::ignore, boost::asio::use_awaitable);
}

boost::asio::awaitable<std::string> Redis::cmd(std::vector<std::string> args) {
    RedisGuard guard(*this, co_await acquire());
    boost::redis::request req;
    for (const auto& arg : args) req.push(arg);
    boost::redis::response<std::string> res;
    co_await guard.get()->async_exec(req, res, boost::asio::use_awaitable);
    co_return std::get<0>(res).value();
}

} // namespace blaze