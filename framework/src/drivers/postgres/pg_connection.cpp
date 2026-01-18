#include <blaze/pg_connection.h>
#include <stdexcept>
#include <libpq-fe.h>

namespace blaze {

    PgConnection::PgConnection(boost::asio::io_context& io_context)
        : ctx_(io_context), conn_(nullptr), socket_(io_context) {}

    PgConnection::~PgConnection() {
        if (conn_) {
            PQfinish(conn_);
            conn_ = nullptr;
        }
    }

    PgConnection::PgConnection(PgConnection&& other) noexcept
        : ctx_(other.ctx_), conn_(other.conn_), socket_(std::move(other.socket_)) {
        other.conn_ = nullptr;
    }

    PgConnection& PgConnection::operator=(PgConnection&& other) noexcept {
        if (this != &other) {
            if (conn_) {
                PQfinish(conn_);
            }
            conn_ = other.conn_;
            socket_ = std::move(other.socket_);
            other.conn_ = nullptr;
        }
        return *this;
    }

    boost::asio::awaitable<void> PgConnection::connect(const std::string& conn_str) {
        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.close(ec);
        }

        if (conn_) {
            PQfinish(conn_);
            conn_ = nullptr;
        }

        // Initiate non-blocking connection
        conn_ = PQconnectStart(conn_str.c_str());
        if (!conn_) {
            throw std::runtime_error("Failed to allocate PostgreSQL connection object");
        }

        if (PQstatus(conn_) == CONNECTION_BAD) {
            throw std::runtime_error("PostgreSQL connection failed: " + std::string(PQerrorMessage(conn_)));
        }

        if (PQsetnonblocking(conn_, 1) == -1) {
            throw std::runtime_error("Failed to set asynchronous PostgreSQL connection");
        }

        // Map the raw FD to our Asio stream descriptor
        const int fd = PQsocket(conn_);
        if (fd == -1) {
            throw std::runtime_error("Failed to get PostgreSQL socket");
        }
        socket_.assign(fd);

        // Loop until OK or FAILED
        PostgresPollingStatusType poll_res;
        while ((poll_res = PQconnectPoll(conn_)) != PGRES_POLLING_OK) {
            if (poll_res == PGRES_POLLING_FAILED) {
                throw std::runtime_error("PostgreSQL handshake failed: " + std::string(PQerrorMessage(conn_)));
            }

            // Suspend here until the socket is ready for the next step
            co_await wait_for_socket(static_cast<int>(poll_res));
        }
    }

    boost::asio::awaitable<PgResult> PgConnection::query(const std::string& sql) {
        // Send query to buffer
        if (!PQsendQuery(conn_, sql.c_str())) {
            throw std::runtime_error("Failed to send query: " + std::string(PQerrorMessage(conn_)));
        }

        // Ensure data is sent to the network
        co_await flush_output();

        // Wait for result
        while (PQisBusy(conn_)) {
            // Wait for data to arrive
            co_await wait_for_socket(PGRES_POLLING_READING);
            
            // Read data into memory
            if (!PQconsumeInput(conn_)) {
                throw std::runtime_error("Failed to consume input: " + std::string(PQerrorMessage(conn_)));
            }
        }

        PGresult* res = PQgetResult(conn_);
        
        // Drain any remaining results
        while (PGresult* extra = PQgetResult(conn_)) {
            PQclear(extra);
        }

        if (!res) {
             throw std::runtime_error("Query returned null result: " + std::string(PQerrorMessage(conn_)));
        }

        auto status = PQresultStatus(res);
        if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
            std::string err = PQresultErrorMessage(res);
            PQclear(res);
            throw std::runtime_error("PostgreSQL Query Error: " + err);
        }

        co_return PgResult(res);
    }

    bool PgConnection::is_connected() const {
        if (!conn_ || PQstatus(conn_) != CONNECTION_OK) return false;
        
        // Force libpq to check the socket for closures
        if (PQconsumeInput(conn_) == 0) return false;
        
        return PQstatus(conn_) == CONNECTION_OK;
    }

    boost::asio::awaitable<void> PgConnection::wait_for_socket(int poll_status) {
        if (poll_status == PGRES_POLLING_READING) {
            co_await socket_.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                boost::asio::use_awaitable);
        } 
        else if (poll_status == PGRES_POLLING_WRITING) {
            co_await socket_.async_wait(boost::asio::posix::stream_descriptor::wait_write,
                boost::asio::use_awaitable);
        }
    }

    boost::asio::awaitable<void> PgConnection::flush_output() {
        while (int ret = PQflush(conn_)) {
            if (ret == -1) {
                throw std::runtime_error("Failed to flush output: " + std::string(PQerrorMessage(conn_)));
            }
            co_await socket_.async_wait(boost::asio::posix::stream_descriptor::wait_write,
                boost::asio::use_awaitable);
        }
    }

} // namespace blaze
