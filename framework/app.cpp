#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <signal.h>
#include <thread>

#include "PyroServer.h"

static App* g_app_instance = nullptr;

App::App() {
    g_app_instance = this;

    // Initialize thread pool
    size_t num_threads = std::thread::hardware_concurrency();
    pool_ = std::make_unique<ThreadPool>(num_threads);

    std::cout << "[App] Thread pool initialized with " << num_threads << " workers\n";
}

App::~App() {
    running_ = false;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
    }
    // pool_ is unique_ptr, will auto-destruct
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

std::string App::handle_request(Request& req, const std::string& client_ip) {
    auto start_time = std::chrono::steady_clock::now();
    Response res;

    int status_code = 500;

    try {
        size_t middleware_index = 0;
        std::function<void()> next = [&]() {
            if (middleware_index < middleware_.size()) {
                auto& mw = middleware_[middleware_index++];
                mw(req, res, next);  // Middleware calls next() to continue
            } else {
                // All middleware done, call route handler
                auto match = router_.match(req.method, req.path);
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
        Response error_res;
        error_res.status(500).json({
            {"error", "Internal Server Error"},
            {"message", e.what()}
        });
        status_code = 500;
        logger_.log_error(std::string("Exception in handle_client: ") + e.what());
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    // logger_.log_access(client_ip, req.method, req.path, status_code, elapsed);
    return res.build_response();
}

void App::signal_handler(int sig) {
    std::cout << "\nShutdown signal received. Stopping server...\n";
    if (g_app_instance) {
        g_app_instance->running_ = false;
        if (g_app_instance->server_fd_ != -1) {
            shutdown(g_app_instance->server_fd_, SHUT_RDWR);
        }
    }
    // Exit immediately - OS will clean up resources
    std::exit(0);
}

void App::listen(int port) {
    signal(SIGINT, App::signal_handler);
    signal(SIGTERM, App::signal_handler);

    std::cout << "[App] Starting on port " << port << "\n";

    PyroServer server(port, this);
    server.run();  // Blocks here in epoll loop
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}

RouteGroup App::group(const std::string& prefix) {
    return RouteGroup(router_, prefix);
}

void App::dispatch_async(const std::function<void()> &task) const {
    pool_->enqueue(task);
}
