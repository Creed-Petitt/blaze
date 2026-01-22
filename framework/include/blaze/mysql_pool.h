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
#include <blaze/database.h>
#include <blaze/mysql_connection.h>

namespace blaze {

class App;

class MySqlPool : public Database, public std::enable_shared_from_this<MySqlPool> {
public:
    explicit MySqlPool(boost::asio::io_context& ctx, std::string url, int size = 10);
    explicit MySqlPool(App& app, std::string url, int size = 10);

    static std::shared_ptr<MySqlPool> open(App& app, std::string url, int size = 10) {
        auto pool = std::make_shared<MySqlPool>(app, std::move(url), size);
        pool->connect();
        return pool;
    }

    void connect();
    boost::asio::awaitable<DbResult> query(const std::string& sql, const std::vector<std::string>& params = {}) override;

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
