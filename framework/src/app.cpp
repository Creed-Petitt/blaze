#include <blaze/app.h>
#include <chrono>
#include <memory>
#include <vector>
#include "server.h"

App::App() {
}

App::~App() {
    // Stop the IO context cleanly
    if(!ioc_.stopped()) {
        ioc_.stop();
    }
}

void App::get(const std::string& path, const Handler &handler) {
    router_.add_route("GET", path, handler);
}

void App::post(const std::string& path, const Handler &handler) {
    router_.add_route("POST", path, handler);
}

void App::put(const std::string& path, const Handler &handler) {
    router_.add_route("PUT", path, handler);
}

void App::del(const std::string& path, const Handler &handler) {
    router_.add_route("DELETE", path, handler);
}

Router &App::get_router() {
    return router_;
}

Logger &App::get_logger() {
    return logger_;
}

std::string App::handle_request(Request& req, const std::string& client_ip, const bool keep_alive) {
    const auto start_time = std::chrono::steady_clock::now();
    Response res;

    int status_code = 500;

    try {
        size_t middleware_index = 0;
        std::function<void()> next = [&]() {
            if (middleware_index < middleware_.size()) {
                const auto& mw = middleware_[middleware_index++];
                mw(req, res, next);
            } else {
                const auto match = router_.match(req.method, req.path);
                if (match.has_value()) {
                    req.params = match->params;
                    try {
                        match->handler(req, res);
                    } catch (const std::exception& e) {
                        res.status(500).json({{"error", e.what()}});
                    }
                } else {
                    res.status(404).send("404 Not Found\n");
                }
            }
        };

        next();
        status_code = res.get_status();

    } catch (const std::exception& e) {
        res.status(500).json({
            {"error", "Internal Server Error"},
            {"message", e.what()}
        });
        status_code = 500;
        logger_.log_error(std::string("Exception in handle_client: ") + e.what());
    }

    if (keep_alive) {
        res.header("Connection", "keep-alive");
    } else {
        res.header("Connection", "close");
    }

    // logger_.log_access(client_ip, req.method, req.path, status_code, elapsed);
    return res.build_response();
}

void App::listen(const int port, int num_threads) {
    if (num_threads <= 0) {
        num_threads = 4;
    }

    auto const address = net::ip::make_address("0.0.0.0");
    auto const endpoint = net::ip::tcp::endpoint{address, static_cast<unsigned short>(port)};

    // Create and launch listening port
    std::make_shared<Listener>(ioc_, endpoint, *this)->run();

    // (Ctrl+C) to stop cleanly
    net::signal_set signals(ioc_, SIGINT, SIGTERM);
    signals.async_wait([this](boost::system::error_code const&, int) {
        ioc_.stop();
    });

    std::cout << "[App] Starting on port " << port << " with " << num_threads << " threads\n";

    // Run the IO Context on n threads
    std::vector<std::thread> v;
    v.reserve(num_threads - 1);
    for(auto i = num_threads - 1; i > 0; --i)
        v.emplace_back([this]{
            ioc_.run();
        });
    
    // Run on the main thread too
    ioc_.run();
    
    for(auto& t : v)
        if(t.joinable()) t.join();
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}

RouteGroup App::group(const std::string& prefix) {
    return RouteGroup(router_, prefix);
}