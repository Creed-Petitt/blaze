#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/websocket.h>
#include <blaze/di.h>
#include <blaze/injector.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <vector>
#include <map>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

namespace blaze {

struct AppConfig {
    size_t max_body_size = 10 * 1024 * 1024; // 10MB default
    int timeout_seconds = 30;                // 30s timeout
    std::string log_path = "stdout";         // Logging destination
};

class App {
private:
    Router router_;
    std::map<std::string, WebSocketHandlers> ws_routes_;
    Logger logger_;
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12};
    std::vector<Middleware> middleware_;
    AppConfig config_;
    ServiceProvider services_;

public:
    App();
    ~App();

    AppConfig& config() { return config_; }
    const AppConfig& get_config() const { return config_; }


    ServiceProvider& services() { return services_; }

    // Register a Singleton (Shared instance)
    // app.provide<MyService>();
    template<typename T>
    void provide() {
        services_.provide<T>();
    }

    // Register a Singleton with Factory
    // app.provide<MyService>([](auto& sp){ return ... });
    template<typename T>
    void provide(std::function<std::shared_ptr<T>(ServiceProvider&)> factory) {
        services_.provide<T>(factory);
    }

    // Register an Existing Instance
    // app.provide(my_ptr);
    template<typename T>
    void provide(std::shared_ptr<T> instance) {
        services_.provide<T>(instance);
    }

    // Register a Transient (New instance per injection)
    // app.provide_transient<MyService>();
    template<typename T>
    void provide_transient() {
        services_.provide_transient<T>();
    }

    
    template<typename Func>
    void get(const std::string& path, Func handler) {
        router_.add_route("GET", path, wrap_handler(handler));
    }

    template<typename Func>
    void post(const std::string& path, Func handler) {
        router_.add_route("POST", path, wrap_handler(handler));
    }

    template<typename Func>
    void put(const std::string& path, Func handler) {
        router_.add_route("PUT", path, wrap_handler(handler));
    }

    template<typename Func>
    void del(const std::string& path, Func handler) {
        router_.add_route("DELETE", path, wrap_handler(handler));
    }

    void ws(const std::string& path, WebSocketHandlers handlers);

    void listen(int port, int num_threads = 0);
    void listen_ssl(int port, const std::string& cert_path, const std::string& key_path, int num_threads = 0);

    void use(const Middleware &mw);

    RouteGroup group(const std::string& prefix);
    Router& get_router();
    const WebSocketHandlers* get_ws_handler(const std::string& path) const;
    Logger& get_logger();
    net::io_context& engine() { return ioc_; }
    boost::asio::awaitable<std::string> handle_request(Request& req, const std::string& client_ip, bool keep_alive);

private:
    boost::asio::awaitable<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler);

    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        return [this, handler](Request& req, Response& res) -> boost::asio::awaitable<void> {
            co_await inject_and_call(const_cast<Func&>(handler), services_, req, res);
        };
    }
};

} // namespace blaze

#endif