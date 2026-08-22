#include <blaze/app.h>

#include <memory>
#include <iostream>

#include "dispatcher.h"
#include "server/server.h"

namespace blaze {

App::App() {
    dispatcher_ = std::make_unique<Dispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, config_);
}

App::App(Config config)
    : config_{std::move(config)} {
    dispatcher_ = std::make_unique<Dispatcher>(router_, middleware_, config_);
    server_ = std::make_unique<Server>(*dispatcher_, config_);
}

App::~App() = default;

void App::spawn(Async<void> task) const {
    server_->spawn(std::move(task));
}

const Router& App::get_router() const {
    check_configuring("Cannot access mutable router");
    return router_;
}

void App::stop() {
    if (state_ == AppState::Running) {
        state_ = AppState::Stopping;
        server_->stop();
    } else if (state_ == AppState::Configuring) {
        state_ = AppState::Stopped;
        server_->stop();
    }
}

void App::listen(const int port) {
    if (state_ != AppState::Configuring) {
        throw std::logic_error("App::listen: App has already started listening or was stopped.");
    }
    state_ = AppState::Running;

    Logger::instance().configure(config_.log_path);
    Logger::instance().set_level(config_.log_level);

    try {
        server_->listen(port, config_.threads);
    } catch (const std::exception& e) {
        state_ = AppState::Stopped;
        std::cerr << "[Blaze] FATAL: Could not start listener: " << e.what() << std::endl;
        throw;
    }
    state_ = AppState::Stopped;
}

void App::use(const Middleware &mw) {
    ensure_configuring("Cannot add middleware");
    middleware_.push_back(mw);
}

} // namespace blaze
