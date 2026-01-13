#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <boost/asio.hpp>
#include <functional>
#include <vector>

namespace net = boost::asio;

class App {
private:
    Router router_;
    Logger logger_;
    net::io_context ioc_;
    std::vector<Middleware> middleware_;

public:
    App();
    ~App();

    void get(const std::string& path, const Handler &handler);
    void post(const std::string& path, const Handler &handler);
    void put(const std::string& path, const Handler &handler);
    void del(const std::string& path, const Handler &handler);

    void listen(int port, int num_threads = 0);  // 0 = use hardware_concurrency

    void use(const Middleware &mw);

    RouteGroup group(const std::string& prefix);

    Router& get_router();

    Logger& get_logger();

    std::string handle_request(Request& req, const std::string& client_ip, bool keep_alive);
};

#endif