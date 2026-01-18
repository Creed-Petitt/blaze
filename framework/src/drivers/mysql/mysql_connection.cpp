#include <blaze/mysql_connection.h>
#include <stdexcept>

namespace blaze {

MySqlConnection::MySqlConnection(boost::asio::io_context& io_context)
    : ctx_(io_context), conn_(mysql_init(nullptr)), socket_(io_context) {
    if (!conn_) throw std::runtime_error("Failed to initialize MySQL object");
}

MySqlConnection::~MySqlConnection() {
    if (conn_) mysql_close(conn_);
}

MySqlConnection::MySqlConnection(MySqlConnection&& other) noexcept
    : ctx_(other.ctx_), conn_(other.conn_), socket_(std::move(other.socket_)) {
    other.conn_ = nullptr;
}

MySqlConnection& MySqlConnection::operator=(MySqlConnection&& other) noexcept {
    if (this != &other) {
        if (conn_) mysql_close(conn_);
        conn_ = other.conn_;
        socket_ = std::move(other.socket_);
        other.conn_ = nullptr;
    }
    return *this;
}

boost::asio::awaitable<void> MySqlConnection::connect(const std::string& host, 
                                                   const std::string& user, 
                                                   const std::string& pass, 
                                                   const std::string& db, 
                                                   unsigned int port) {
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.close(ec);
    }

    if (conn_) {
        mysql_close(conn_);
    }
    conn_ = mysql_init(nullptr);
    if (!conn_) throw std::runtime_error("Failed to re-initialize MySQL object");

    mysql_options(conn_, MYSQL_OPT_NONBLOCK, 0);

    MYSQL* res;
    int status = mysql_real_connect_start(&res, conn_, host.c_str(), user.c_str(), 
                                        pass.c_str(), db.c_str(), port, nullptr, 0);

    while (status) {
        int fd = mysql_get_socket(conn_);
        if (fd >= 0 && !socket_.is_open()) {
            socket_.assign(fd);
        }
        
        if (socket_.is_open()) {
            co_await wait_for_socket(status);
        } else {
            // If we don't have a socket yet, we have to wait a tiny bit
            boost::asio::steady_timer timer(ctx_, std::chrono::milliseconds(1));
            co_await timer.async_wait(boost::asio::use_awaitable);
        }
        status = mysql_real_connect_cont(&res, conn_, status);
    }

    if (!res) throw std::runtime_error("MySQL Connection Failed: " + std::string(mysql_error(conn_)));

    if (!socket_.is_open()) {
        socket_.assign(mysql_get_socket(conn_));
    }
}

boost::asio::awaitable<MySqlResult> MySqlConnection::query(const std::string& sql) {
    int err;
    int status = mysql_real_query_start(&err, conn_, sql.c_str(), sql.length());

    while (status) {
        co_await wait_for_socket(status);
        status = mysql_real_query_cont(&err, conn_, status);
    }

    if (err) throw std::runtime_error("MySQL Query Error: " + std::string(mysql_error(conn_)));

    MYSQL_RES* res_ptr;
    status = mysql_store_result_start(&res_ptr, conn_);
    while (status) {
        co_await wait_for_socket(status);
        status = mysql_store_result_cont(&res_ptr, conn_, status);
    }

    co_return MySqlResult(conn_, res_ptr);
}

bool MySqlConnection::is_connected() const {
    return conn_ && mysql_ping(conn_) == 0;
}

boost::asio::awaitable<void> MySqlConnection::wait_for_socket(int status) {
    if (status & MYSQL_WAIT_READ) {
        co_await socket_.async_wait(boost::asio::posix::stream_descriptor::wait_read, boost::asio::use_awaitable);
    } else if (status & MYSQL_WAIT_WRITE) {
        co_await socket_.async_wait(boost::asio::posix::stream_descriptor::wait_write, boost::asio::use_awaitable);
    }
}

} // namespace blaze
