#ifndef SERVER_H
#define SERVER_H

#include <boost/beast/core.hpp>         // buffer, tcp_stream
#include <boost/beast/http.hpp>         // request, response, parsing
#include <boost/asio/ip/tcp.hpp>        // sockets, acceptor
#include <boost/asio.hpp>               // io_context
#include <boost/asio/ssl.hpp>           // ssl
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace blaze {

class App;

// Handles HTTP server connection
class Session : public std::enable_shared_from_this<Session> {
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::optional<http::request_parser<http::string_body>> parser_;
    App& app_;

public:
    Session(tcp::socket&& socket, App& app);
    void run();
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);
};

// Handles HTTPS server connection
class SslSession : public std::enable_shared_from_this<SslSession> {
    ssl::stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    std::optional<http::request_parser<http::string_body>> parser_;
    App& app_;

public:
    SslSession(tcp::socket&& socket, ssl::context& ctx, App& app);
    void run();
    void on_run();
    void on_handshake(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);
    void do_shutdown();
    void on_shutdown(beast::error_code ec);
};

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    App& app_;

public:
    Listener(net::io_context& ioc, const tcp::endpoint &endpoint, App& app);
    void run();
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};

// Accepts incoming HTTPS connections
class SslListener : public std::enable_shared_from_this<SslListener> {
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
    App& app_;

public:
    SslListener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint &endpoint, App& app);
    void run();
    void do_accept();
        void on_accept(beast::error_code ec, tcp::socket socket);
    };
    
    } // namespace blaze
    
    #endif
    