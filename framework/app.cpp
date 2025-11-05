#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <memory>
#include <signal.h>
#include <thread>
#include <vector>
#include <utility>
#include <stdexcept>

#include "HttpServer.h"

static App* g_app_instance = nullptr;

App::App() {
    g_app_instance = this;

    // Initialize thread pool
    size_t num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 4;
    }
    pool_ = std::make_unique<ThreadPool>(num_threads, 1024);

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

Logger &App::get_logger() {
    return logger_;
}

std::string App::handle_request(Request& req, const std::string& client_ip, bool keep_alive) {
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
        res.status(500).json({
            {"error", "Internal Server Error"},
            {"message", e.what()}
        });
        status_code = 500;
        logger_.log_error(std::string("Exception in handle_client: ") + e.what());
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    // Set Connection header based on what PyroServer decided
    if (keep_alive) {
        res.header("Connection", "keep-alive");
    } else {
        res.header("Connection", "close");
    }

    // logger_.log_access(client_ip, req.method, req.path, status_code, elapsed);
    return res.build_response();
}

void App::signal_handler(int sig) {
    if (g_app_instance) {
        g_app_instance->running_ = false;
    }
}

void App::listen(int port, size_t num_threads) {
    signal(SIGINT, App::signal_handler);
    signal(SIGTERM, App::signal_handler);

    active_connections_.store(0, std::memory_order_relaxed);

    // Default to number of CPU cores
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;  // Fallback if detection fails
    }

    std::cout << "[App] Starting on port " << port << " with " << num_threads << " event loop threads\n";

    int listen_fd = HttpServer::create_listening_socket(port);
    server_fd_ = listen_fd;

    running_.store(true, std::memory_order_release);

    std::vector<std::shared_ptr<HttpServer>> servers;
    servers.reserve(num_threads);
    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        auto server = std::make_shared<HttpServer>(port, this, listen_fd, false,
                                                    &active_connections_, max_connections_);
        servers.emplace_back(server);
        workers.emplace_back([server, i]() {
            std::cout << "[App] Event loop thread " << i << " starting\n";
            server->run();
        });
    }

    while (running_.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto& server : servers) {
        server->stop();
    }

    shutdown(listen_fd, SHUT_RDWR);

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    close(listen_fd);
    server_fd_ = -1;
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}

RouteGroup App::group(const std::string& prefix) {
    return RouteGroup(router_, prefix);
}

bool App::dispatch_async(std::function<void()> task) const {
    return pool_->try_enqueue(std::move(task));
}
