#pragma once

#include <blaze/websocket.h>
#include <blaze/websocket_registry.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace blaze {

template<class Stream>
class WebSocketSession : public WebSocket, public std::enable_shared_from_this<WebSocketSession<Stream>> {
    websocket::stream<Stream> ws_;
    beast::flat_buffer buffer_;
    const WebSocketHandlers& handlers_;
    WebSocketRegistry& registry_;
    std::string target_;

    std::queue<std::string> write_queue_;
    std::mutex queue_mutex_;

public:
    explicit WebSocketSession(
        Stream&& stream,
        const WebSocketHandlers& handlers,
        WebSocketRegistry& registry,
        std::string target);

    void run(http::request<http::string_body> req);

    void on_accept(beast::error_code ec);
    void do_read();
    void on_read(const beast::error_code& ec, std::size_t bytes_transferred);

    void send(std::string message) override;
    void close() override;

private:
    void do_write();
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
};

} // namespace blaze
