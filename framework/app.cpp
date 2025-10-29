#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <signal.h>
#include <thread>

static App* g_app_instance = nullptr;

App::App() {
    g_app_instance = this;
}

App::~App() {
    if (server_fd_ != -1) {
        close(server_fd_);
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

void App::handle_client(int client_fd, const std::string& client_ip) {
    auto start_time = std::chrono::steady_clock::now();

    Request req(client_fd);
    Response res;

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

    res.write(client_fd);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time).count();

    logger_.log_access(client_ip, req.method, req.path, res.get_status(), elapsed);
    close(client_fd);
}

void App::signal_handler(int sig) {
    std::cout << "\nShutdown signal received (signal " << sig << ")\n";
    if (g_app_instance) {
        g_app_instance->running_ = false;
        if (g_app_instance->server_fd_ != -1) {
            shutdown(g_app_instance->server_fd_, SHUT_RDWR);
            close(g_app_instance->server_fd_);
        }
    }
}

void App::listen(int port) {
    signal(SIGINT, App::signal_handler);
    signal(SIGTERM, App::signal_handler);

    size_t num_threads = std::thread::hardware_concurrency();
    pool_ = std::make_unique<ThreadPool>(num_threads);

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd_ == -1) {
        perror("socket");
        return;
    }

    std::cout << "Server starting on port " << port << " with " << num_threads << " worker threads\n";
    std::cout << "Socket created successfully (fd=" << server_fd_ << ")\n";

    int yes = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        return;
    }

    sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd_, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        return;
    }

    if (::listen(server_fd_, 128) == -1) {
        perror("listen");
        return;
    }

    sockaddr_in client_addr {};
    socklen_t client_len = sizeof(client_addr);

    while (running_) {
        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr *>(&client_addr), &client_len);

        if (!running_) break;

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        std::string client_ip_str(client_ip);

        pool_->enqueue([this, client_fd, client_ip_str]() {
            handle_client(client_fd, client_ip_str);
        });
    }
    std::cout << "Server stopped. Cleaning up...\n";
}

void App::use(const Middleware &mw) {
    middleware_.push_back(mw);
}
