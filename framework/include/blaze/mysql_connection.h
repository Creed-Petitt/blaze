#ifndef BLAZE_MYSQL_CONNECTION_H
#define BLAZE_MYSQL_CONNECTION_H

#include <mysql.h>
#include <blaze/mysql_result.h>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <memory>

namespace blaze {

class MySqlConnection {
public:
    explicit MySqlConnection(boost::asio::io_context& io_context);
    ~MySqlConnection();
    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;
    MySqlConnection(MySqlConnection&&) noexcept;
    MySqlConnection& operator=(MySqlConnection&&) noexcept;

    boost::asio::awaitable<void> connect(const std::string& host, 
        const std::string& user,
        const std::string& pass,
        const std::string& db,
        unsigned int port = 3306);

    boost::asio::awaitable<MySqlResult> query(const std::string& sql, const std::vector<std::string>& params = {});

    [[nodiscard]] bool is_connected() const;
    [[nodiscard]] bool is_open() const { return socket_.is_open(); }
    void force_close() { if (socket_.is_open()) { boost::system::error_code ec; socket_.close(ec); } }

private:
    boost::asio::awaitable<void> wait_for_socket(int status);
    std::string format_query(const std::string& sql, const std::vector<std::string>& params);

    boost::asio::io_context& ctx_;
    MYSQL* conn_;
    boost::asio::posix::stream_descriptor socket_;
};

} // namespace blaze

#endif // BLAZE_MYSQL_CONNECTION_H
