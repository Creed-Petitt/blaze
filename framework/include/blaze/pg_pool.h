#ifndef BLAZE_POSTGRES_POOL_H
#define BLAZE_POSTGRES_POOL_H

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include "pg_connection.h"

namespace blaze {

    class App;

    class PgPool {
    public:
        // Added default size = 10 to match Redis and MySQL
        explicit PgPool(boost::asio::io_context& ctx, std::string conn_str, int size = 10);
        explicit PgPool(App& app, std::string conn_str, int size = 10);

        void connect();

        // High-level query API
        boost::asio::awaitable<PgResult> query(const std::string& sql);

    private:
        boost::asio::io_context& ctx_;
        std::string conn_str_;
        int size_;

        std::vector<std::unique_ptr<PgConnection>> pool_;
        std::queue<PgConnection*> available_;
        std::queue<std::shared_ptr<boost::asio::steady_timer>> waiters_;
        std::mutex mutex_;

        boost::asio::awaitable<PgConnection*> acquire();
        void release(PgConnection* conn);
        boost::asio::awaitable<void> start();
    };

    // Shortcut
    using Postgres = PgPool;

} // namespace blaze

#endif // BLAZE_POSTGRES_POOL_H