#ifndef BLAZE_POSTGRES_CONNECTION_H
#define BLAZE_POSTGRES_CONNECTION_H

#include <libpq-fe.h>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <string>
#include <blaze/pg_result.h>

namespace blaze {

    class PgConnection {
    public:
        explicit PgConnection(boost::asio::io_context& io_context);
        ~PgConnection();

        PgConnection(const PgConnection&) = delete;
        PgConnection& operator=(const PgConnection&) = delete;

        PgConnection(PgConnection&& other) noexcept;
        PgConnection& operator=(PgConnection&& other) noexcept;

        // Async Operations
        boost::asio::awaitable<void> connect(const std::string& conn_str);
        
        [[nodiscard]] boost::asio::awaitable<PgResult> query(const std::string& sql);

        [[nodiscard]] bool is_connected() const;
        [[nodiscard]] bool is_open() const { return socket_.is_open(); }
        void force_close() { if (socket_.is_open()) { boost::system::error_code ec; socket_.close(ec); } }

    private:
        boost::asio::io_context& ctx_;
        PGconn* conn_;
        boost::asio::posix::stream_descriptor socket_;


        boost::asio::awaitable<void> wait_for_socket(int poll_status);
        boost::asio::awaitable<void> flush_output();
    };

} // namespace blaze

#endif // BLAZE_POSTGRES_CONNECTION_H
