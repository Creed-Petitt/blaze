#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <vector>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

namespace blaze {

class App {
private:
    Router router_;
    Logger logger_;
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12};
    std::vector<Middleware> middleware_;

public:
    App();
    ~App();

    void get(const std::string& path, const Handler &handler);
    void post(const std::string& path, const Handler &handler);
    void put(const std::string& path, const Handler &handler);
    void del(const std::string& path, const Handler &handler);

    void listen(int port, int num_threads = 0);  // 0 = use hardware_concurrency
    void listen_ssl(int port, const std::string& cert_path, const std::string& key_path, int num_threads = 0);

    void use(const Middleware &mw);

    RouteGroup group(const std::string& prefix);

    Router& get_router();

    Logger& get_logger();

    net::io_context& engine() { return ioc_; }

    boost::asio::awaitable<std::string> handle_request(Request& req, const std::string& client_ip, bool keep_alive);
};

} // namespace blaze

#endif