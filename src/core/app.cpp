#include <blaze/app.h>

#include <chrono>
#include <memory>
#include <iostream>

#include "../request/request_dispatcher.h"
#include "../server/server.h"

namespace blaze {

App::App() {
    dispatcher_ = std::make_unique<RequestDispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, websockets_, config_);
}

App::App(Config config) : config_{std::move(config)} {
    dispatcher_ = std::make_unique<RequestDispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, websockets_, config_);
}

App::~App() = default;

void App::ws(const std::string& path, WebSocketHandlers handlers) {
    websockets_.add_route(path, std::move(handlers));
}

void App::spawn(Async<void> task) const
{
    server_->spawn(std::move(task));
}

void App::broadcast_raw(const std::string& path, const std::string& payload) {
    websockets_.broadcast(path, payload);
}

Router& App::get_router() {
    return router_;
}

void App::stop() {
    websockets_.close_all();
    server_->stop();
}

void App::listen(const int port, const int num_threads) const {
    Logger::instance().configure(config_.log_path);
    Logger::instance().set_level(config_.log_level);

    try {
        server_->listen(port, num_threads);
    } catch (const std::exception& e) {
        std::cerr << "[Blaze] FATAL: Could not start listener: " << e.what() << std::endl;
        throw;
    }
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}

RouteGroup App::group(const std::string& prefix) {
    return RouteGroup(router_, prefix);
}

} // namespace blaze
