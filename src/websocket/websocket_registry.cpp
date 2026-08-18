#include <blaze/websocket_registry.h>

#include <ranges>
#include <utility>

namespace blaze {

void WebSocketRegistry::add_route(const std::string& path, WebSocketHandlers handlers) {
    std::lock_guard lock(mutex_);
    routes_[path] = std::move(handlers);
}

const WebSocketHandlers* WebSocketRegistry::find_handler(const std::string& path) const {
    std::lock_guard lock(mutex_);
    const auto it = routes_.find(path);
    if (it == routes_.end()) {
        return nullptr;
    }
    return &it->second;
}

void WebSocketRegistry::register_session(const std::string& path, const std::shared_ptr<WebSocket>& ws) {
    std::lock_guard lock(mutex_);
    sessions_[path].push_back(ws);
}

void WebSocketRegistry::broadcast(const std::string& path, const std::string& payload) {
    std::lock_guard lock(mutex_);
    const auto it = sessions_.find(path);
    if (it == sessions_.end()) {
        return;
    }

    auto& sessions = it->second;
    for (auto session = sessions.begin(); session != sessions.end(); ) {
        if (const auto ws = session->lock()) {
            try {
                ws->send(payload);
                ++session;
            } catch (...) {
                session = sessions.erase(session);
            }
        } else {
            session = sessions.erase(session);
        }
    }
}

void WebSocketRegistry::close_all() {
    std::lock_guard lock(mutex_);
    for (auto& sessions : sessions_ | std::views::values) {
        for (auto& weak_ws : sessions) {
            if (const auto ws = weak_ws.lock()) {
                ws->close();
            }
        }
    }
}

} // namespace blaze
