#ifndef BLAZE_MYSQL_POOL_H
#define BLAZE_MYSQL_POOL_H

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <blaze/json.h>
#include <blaze/db_common.h>
#include <blaze/mysql_connection.h>

namespace blaze {

class App;

class MySqlPool {
public:
    explicit MySqlPool(boost::asio::io_context& ctx, std::string url, int size = 10);
    explicit MySqlPool(App& app, std::string url, int size = 10);

    void connect();
    boost::asio::awaitable<Json> query(const std::string& sql);

private:
    boost::asio::io_context& ctx_;
    std::string url_;
    int size_;

    struct Config {
        std::string host, user, pass, db;
        unsigned int port;
    } config_;

    std::vector<std::unique_ptr<MySqlConnection>> pool_;
    std::queue<MySqlConnection*> available_;
    std::queue<std::shared_ptr<boost::asio::steady_timer>> waiters_;
    std::mutex mutex_;
    CircuitBreaker breaker_;

    void parse_url();
    boost::asio::awaitable<MySqlConnection*> acquire();
    void release(MySqlConnection* conn);
    boost::asio::awaitable<void> start();
};

} // namespace blaze

#endif
