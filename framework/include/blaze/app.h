#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/thread_pool.h>
#include <atomic>
#include <memory>
#include <functional>

class App {
private:
    Router router_;
    Logger logger_;
    std::unique_ptr<ThreadPool> pool_;
    int server_fd_ = -1;
    std::atomic<bool> running_{true};
    std::atomic<size_t> active_connections_{0};
    const size_t max_connections_ = 10000;
    std::vector<Middleware> middleware_;

    static void signal_handler(int sig);

public:
    App();
    ~App();

    void get(const std::string& path, const Handler &handler);
    void post(const std::string& path, const Handler &handler);
    void put(const std::string& path, const Handler &handler);
    void del(const std::string& path, const Handler &handler);

    void listen(int port, size_t num_threads = 0);  // 0 = use hardware_concurrency

    void use(const Middleware &mw);

    RouteGroup group(const std::string& prefix);

    Router& get_router();

    Logger& get_logger();

    bool dispatch_async(std::function<void()> task) const;

    std::string handle_request(Request& req, const std::string& client_ip, bool keep_alive);
};

#endif
