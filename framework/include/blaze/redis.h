#ifndef BLAZE_REDIS_H
#define BLAZE_REDIS_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <boost/asio/awaitable.hpp>

namespace boost::redis { class connection; }

namespace blaze {

class App;
class RedisGuard; // Forward declaration

class Redis {
public:
    explicit Redis(App& app, std::string host = "127.0.0.1", std::string port = "6379", int pool_size = 10);
    ~Redis();

    void connect();

    boost::asio::awaitable<std::string> get(std::string_view key);
    boost::asio::awaitable<void> set(std::string_view key, std::string_view value, int expire_seconds = 0);
    boost::asio::awaitable<void> del(std::string_view key);
    boost::asio::awaitable<std::string> cmd(std::vector<std::string> args);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    boost::asio::awaitable<boost::redis::connection*> acquire();
    void release(boost::redis::connection* conn);

    friend class RedisGuard; // Allow guard to release connections
};

} // namespace blaze

#endif