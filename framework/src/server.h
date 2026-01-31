#ifndef SERVER_H
#define SERVER_H

#include <boost/beast/core.hpp>         // buffer, tcp_stream
#include <boost/beast/http.hpp>         // request, response, parsing
#include <boost/beast/websocket.hpp>    // websocket
#include <boost/beast/websocket/ssl.hpp> // SSL websocket support
#include <boost/asio/ip/tcp.hpp>        // sockets, acceptor
#include <boost/asio.hpp>               // io_context
#include <boost/asio/ssl.hpp>           // ssl
#include <memory>
#include <queue>
#include <mutex>
#include <optional>
#include <utility>
#include <blaze/websocket.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

namespace blaze {

class App;

// Handles a WebSocket connection
template<class Stream>
class WebSocketSession : public WebSocket, public std::enable_shared_from_this<WebSocketSession<Stream>> {
    websocket::stream<Stream> ws_;
    beast::flat_buffer buffer_;
    const WebSocketHandlers& handlers_;
    App& app_;
    std::string target_;

    std::queue<std::string> write_queue_;
    std::mutex queue_mutex_;

public:
    // Resolver for Stream type (TCP vs SSL)
    explicit WebSocketSession(Stream&& stream, const WebSocketHandlers& handlers, App& app, std::string target);

    void run(http::request<http::string_body> req);

    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    
    // WebSocket Interface overrides
    void send(std::string message) override;
    void close() override;

    void do_write();
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};

// Handles HTTP server connection (templated for TCP or SSL)
template<class Stream>
class HttpSession : public std::enable_shared_from_this<HttpSession<Stream>> {
    Stream stream_;
    beast::flat_buffer buffer_;
    std::optional<http::request_parser<http::string_body>> parser_;
    App& app_;

public:
    template<typename... Args>
    HttpSession(App& app, Args&&... args);
    
    void run();
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred);
    
    // SSL-specific shutdown handling
    void do_shutdown();
    void on_shutdown(beast::error_code ec);

    Stream& stream() { return stream_; }

private:
    std::string get_client_ip();
    void send_error_response(http::status status, std::string_view message);
    bool try_websocket_upgrade();
};

// Type aliases for cleaner usage in listeners
using Session = HttpSession<beast::tcp_stream>;
using SslSession = HttpSession<ssl::stream<beast::tcp_stream>>;

// Base class for listeners to allow polymorphic tracking for shutdown
class ListenerBase {
public:
    virtual ~ListenerBase() = default;
    virtual void stop() = 0;
};

// Accepts incoming connections and launches the sessions
class Listener : public ListenerBase, public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    App& app_;

public:
    Listener(net::io_context& ioc, const tcp::endpoint &endpoint, App& app);
    void run();
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
    void stop() override;
};

// Accepts incoming HTTPS connections
class SslListener : public ListenerBase, public std::enable_shared_from_this<SslListener> {
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;
    App& app_;

public:
    SslListener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint &endpoint, App& app);
    void run();
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
    void stop() override;
};

} // namespace blaze

#endif