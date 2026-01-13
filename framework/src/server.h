#ifndef SERVER_H
#define SERVER_H

#include <boost/beast/core.hpp>         // buffer, tcp_stream
#include <boost/beast/http.hpp>         // request, response, parsing
#include <boost/asio/ip/tcp.hpp>        // sockets, acceptor
#include <boost/asio.hpp>               // io_context
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class App;

// Handles HTTP server connection
class Session : public std::enable_shared_from_this<Session> {
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    App& app_;

public:
    Session(tcp::socket&& socket, App& app);
    void run();
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);
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

#endif