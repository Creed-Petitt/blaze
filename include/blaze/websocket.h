#ifndef BLAZE_WEBSOCKET_H
#define BLAZE_WEBSOCKET_H

#include <string>
#include <functional>
#include <memory>
#include <variant>

namespace blaze {

class WebSocket {
public:
    virtual ~WebSocket() = default;

    virtual void send(std::string message) = 0;

    virtual void close() = 0;

    void* user_data = nullptr;
};

// Events
using WebSocketOpenHandler = std::function<void(std::shared_ptr<WebSocket>)>;
using WebSocketMessageHandler = std::function<void(std::shared_ptr<WebSocket>, std::string)>;
using WebSocketCloseHandler = std::function<void(std::shared_ptr<WebSocket>)>;

struct WebSocketHandlers {
    WebSocketOpenHandler on_open;
    WebSocketMessageHandler on_message;
    WebSocketCloseHandler on_close;
};

} // namespace blaze

#endif // BLAZE_WEBSOCKET_H