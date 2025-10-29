#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include "router.h"
#include "../thread_pool.h"
#include "../logger.h"
#include <atomic>
#include <memory>

class App {
private:
    Router router_;
    Logger logger_;
    std::unique_ptr<ThreadPool> pool_;
    int server_fd_ = -1;
    std::atomic<bool> running_{true};
    std::vector<Middleware> middleware_;

    void handle_client(int client_fd, const std::string& client_ip);
    void setup_signal_handlers();
    static void signal_handler(int sig);

public:
    App();
    ~App();

    void get(const std::string& path, const Handler &handler);
    void post(const std::string& path, const Handler &handler);
    void put(const std::string& path, const Handler &handler);
    void del(const std::string& path, const Handler &handler);

    void listen(int port);

    void use(const Middleware &mw);

    Router& get_router();
};

#endif