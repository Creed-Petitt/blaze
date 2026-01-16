#ifndef BLAZE_PG_POOL_H
#define BLAZE_PG_POOL_H

#include <blaze/pg_connection.h>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <queue>

namespace blaze {

    class App; // Forward declaration

    class PgPool {
    public:
        explicit PgPool(boost::asio::io_context& ctx, std::string conn_str, int size);
        
        // Convenience Constructor
        explicit PgPool(App& app, std::string conn_str, int size);

        PgPool(const PgPool&) = delete;
        PgPool& operator=(const PgPool&) = delete;

        boost::asio::awaitable<void> start();

        void connect();

        [[nodiscard]] boost::asio::awaitable<PgResult> query(const std::string& sql);

    private:
        [[nodiscard]] boost::asio::awaitable<PgConnection*> acquire();
        void release(PgConnection* conn);

        boost::asio::io_context& ctx_;
        std::string conn_str_;
        int size_;

        std::vector<std::unique_ptr<PgConnection>> pool_;
        std::queue<PgConnection*> available_;
        std::queue<std::shared_ptr<boost::asio::steady_timer>> waiters_;
        std::mutex mutex_;
    };

} // namespace blaze

#endif // BLAZE_PG_POOL_H
