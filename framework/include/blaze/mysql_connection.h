#ifndef BLAZE_MYSQL_CONNECTION_H
#define BLAZE_MYSQL_CONNECTION_H

#include <mariadb/mysql.h>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <string>
#include <blaze/mysql_result.h>

namespace blaze {

class MySqlConnection {
public:
    explicit MySqlConnection(boost::asio::io_context& io_context);
    ~MySqlConnection();

    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;
    MySqlConnection(MySqlConnection&& other) noexcept;
    MySqlConnection& operator=(MySqlConnection&& other) noexcept;

    boost::asio::awaitable<void> connect(const std::string& host, 
                                       const std::string& user, 
                                       const std::string& pass, 
                                       const std::string& db, 
                                       unsigned int port);
    
    boost::asio::awaitable<MySqlResult> query(const std::string& sql);
    bool is_connected() const;

private:
    boost::asio::io_context& ctx_;
    MYSQL* conn_;
    boost::asio::posix::stream_descriptor socket_;

    boost::asio::awaitable<void> wait_for_socket(int status);
};

} // namespace blaze

#endif