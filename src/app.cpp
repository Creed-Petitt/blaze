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

Router& App::get_router() 
{
    return router_;
}

void App::stop() const
{
    server_->stop();
}

void App::listen(const int port) const {
    Logger::instance().configure(config_.log_path);
    Logger::instance().set_level(config_.log_level);

    try {
        server_->listen(port, config_.threads);
    } catch (const std::exception& e) {
        std::cerr << "[Blaze] FATAL: Could not start listener: " << e.what() << std::endl;
        throw;
    }
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}

} // namespace blaze
