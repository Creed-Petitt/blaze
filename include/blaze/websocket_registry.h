#pragma once

#include <blaze/websocket.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace blaze {

class WebSocketRegistry {
    std::map<std::string, WebSocketHandlers> routes_;
    std::map<std::string, std::vector<std::weak_ptr<WebSocket>>> sessions_;
    mutable std::mutex mutex_;

public:
    void add_route(const std::string& path, WebSocketHandlers handlers);
    const WebSocketHandlers* find_handler(const std::string& path) const;
    void register_session(const std::string& path, const std::shared_ptr<WebSocket>& ws);
    void broadcast(const std::string& path, const std::string& payload);
    void close_all();
};

} // namespace blaze
