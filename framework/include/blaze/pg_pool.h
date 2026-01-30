#ifndef BLAZE_POSTGRES_POOL_H
#define BLAZE_POSTGRES_POOL_H

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
#include <blaze/app.h>
#include "pg_connection.h"

namespace blaze {

    class PgPool : public Database, public std::enable_shared_from_this<PgPool> {
    public:
        explicit PgPool(boost::asio::io_context& ctx, std::string conn_str, int size = 10);
        explicit PgPool(App& app, std::string conn_str, int size = 10);

        static std::shared_ptr<PgPool> open(App& app, std::string conn_str, int size = 10) {
            auto pool = std::make_shared<PgPool>(app, std::move(conn_str), size);
            pool->connect();
            return pool;
        }

        static void install(App& app, std::string conn_str, int size = 10) {
            app.service(open(app, std::move(conn_str), size)).template as<Database>();
        }

        void connect();
        boost::asio::awaitable<DbResult> query(const std::string& sql, const std::vector<std::string>& params = {}) override;
        boost::asio::awaitable<void> execute_transaction(std::function<boost::asio::awaitable<void>(Database&)> block) override;
        
        std::string placeholder(const int index) const override {
            return "$" + std::to_string(index);
        }

    private:
        boost::asio::io_context& ctx_;
        std::string conn_str_;
        int size_;

        std::vector<std::unique_ptr<PgConnection>> pool_;
        std::queue<PgConnection*> available_;
        std::queue<std::shared_ptr<boost::asio::steady_timer>> waiters_;
        std::mutex mutex_;
        CircuitBreaker breaker_;

        boost::asio::awaitable<PgConnection*> acquire();
        void release(PgConnection* conn);
        boost::asio::awaitable<void> start();
    };

    // Shortcut
    using Postgres = PgPool;

} // namespace blaze

#endif // BLAZE_POSTGRES_POOL_H