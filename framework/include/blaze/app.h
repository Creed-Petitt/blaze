#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/websocket.h>
#include <blaze/di.h>
#include <blaze/injector.h>
#include <blaze/json.h>
#include <blaze/reflection.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <vector>
#include <map>
#include <mutex>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

namespace blaze {

template<typename T>
struct extract_async_type {
    using type = void;
    static constexpr bool is_async = false;
};

template<typename T>
struct extract_async_type<boost::asio::awaitable<T>> {
    using type = T;
    static constexpr bool is_async = true;
};

struct AppConfig {
    size_t max_body_size = 10 * 1024 * 1024; // 10MB default
    int timeout_seconds = 30;                // 30s timeout
    std::string log_path = "stdout";         // Logging destination
};


/**
 * @brief The primary entry point for a Blaze application.
 * 
 * The App class manages the internal Boost.Asio engine, dependency injection via ServiceProvider,
 * and HTTP routing. It supports 'magic' dependency injection directly into route handlers.
 */
class App {
private:
    Router router_;
    std::map<std::string, WebSocketHandlers> ws_routes_;

    // Session tracking for path-based broadcasting
    std::map<std::string, std::vector<std::weak_ptr<WebSocket>>> ws_sessions_;
    std::mutex ws_mtx_;

    void broadcast_raw(const std::string& path, const std::string& payload);

    Logger logger_;
    net::io_context ioc_;
    ssl::context ssl_ctx_{ssl::context::tlsv12};
    std::vector<Middleware> middleware_;
    AppConfig config_;
    ServiceProvider services_;

public:
    App();
    ~App();

    /**
     * @brief Helper class for fluent service registration.
     */
    template<typename T>
    class ServiceRegistration {
    private:
        App& app_;
        std::shared_ptr<T> instance_;

    public:
        ServiceRegistration(App& app, std::shared_ptr<T> instance)
            : app_(app), instance_(instance) {
            // Auto-register as concrete type
            app_.provide<T>(instance_);
        }

        // Register as additional interface(s)
        template<typename Interface>
        ServiceRegistration& as() {
            app_.provide<Interface>(instance_);
            return *this;
        }

        // Allow access to underlying instance if needed
        std::shared_ptr<T> get() const { return instance_; }
    };

    /**
     * @brief Registers a service instance with fluent syntax.
     * usage: app.service(Postgres::open(...)).as<Database>();
     */
    template<typename T>
    ServiceRegistration<T> service(std::shared_ptr<T> instance) {
        return ServiceRegistration<T>(*this, instance);
    }

    /**
     * @brief Access the application configuration.
     */
    AppConfig& config() { return config_; }
    const AppConfig& get_config() const { return config_; }

    App& log_to(const std::string& path) { config_.log_path = path; return *this; }
    App& max_body_size(size_t bytes) { config_.max_body_size = bytes; return *this; }

    /**
     * @brief Access the internal ServiceProvider for registering dependencies.
     */
    ServiceProvider& services() { return services_; }

    /**
     * @brief Resolves a service from the internal DI container.
     * Shortcut for app.services().resolve<T>().
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        return services_.resolve<T>();
    }

    /**
     * @brief Registers an auto-wired singleton service in the DI container.
     */
    template<typename T>
    void provide() {
        services_.provide<T>();
    }

    /**
     * @brief Registers a singleton service with a custom factory.
     */
    template<typename T>
    void provide(std::function<std::shared_ptr<T>(ServiceProvider&)> factory) {
        services_.provide<T>(factory);
    }

    /**
     * @brief Registers an existing instance as a singleton.
     */
    template<typename T>
    void provide(std::shared_ptr<T> instance) {
        services_.provide<T>(instance);
    }

    /**
     * @brief Registers an auto-wired transient service (new instance every time).
     */
    template<typename T>
    void provide_transient() {
        services_.provide_transient<T>();
    }

    /**
     * @brief Registers a GET route.
     * 
     * The handler function supports injection. You can request any registered
     * service, as well as 'Request&' or 'Response&', as arguments.
     * 
     * @param path The URL path (e.g., "/users/:id").
     * @param handler The callback function or lambda.
     */
    template<typename Func>
    void get(const std::string& path, Func handler) {
        router_.add_doc(reflection::inspect_handler<Func>("GET", path));
        router_.add_route("GET", path, wrap_handler(handler));
    }

    /** @brief Registers a POST route with magic injection. */
    template<typename Func>
    void post(const std::string& path, Func handler) {
        router_.add_doc(reflection::inspect_handler<Func>("POST", path));
        router_.add_route("POST", path, wrap_handler(handler));
    }

    /** @brief Registers a PUT route with magic injection. */
    template<typename Func>
    void put(const std::string& path, Func handler) {
        router_.add_doc(reflection::inspect_handler<Func>("PUT", path));
        router_.add_route("PUT", path, wrap_handler(handler));
    }

    /** @brief Registers a DELETE route with magic injection. */
    template<typename Func>
    void del(const std::string& path, Func handler) {
        router_.add_doc(reflection::inspect_handler<Func>("DELETE", path));
        router_.add_route("DELETE", path, wrap_handler(handler));
    }

    /** @brief Registers a WebSocket route. */
    void ws(const std::string& path, WebSocketHandlers handlers);

    /**
     * @brief Broadcasts a message to all connected WebSockets on a specific path.
     * Automatically handles serialization and dead connection pruning.
     */
    template<typename T>
    void broadcast(const std::string& path, const T& data) {
        broadcast_raw(path, Json(data).dump());
    }

    /**
     * @brief Internal: WebSocket session management (used by server).
     */
    void _register_ws(const std::string& path, std::shared_ptr<WebSocket> ws);

    /** @brief Starts a background task (coroutine) in the event loop. */
    void spawn(Async<void> task);

    // Getters for internal state (used by drivers/handlers)
    /**
     * @brief Starts the HTTP server on the specified port.
     * 
     * @param port The port to listen on.
     * @param num_threads Number of threads for the event loop (0 = auto-detect).
     */
    void listen(int port, int num_threads = 0);

    /**
     * @brief Starts the HTTPS (SSL) server.
     */
    void listen_ssl(int port, const std::string& cert_path, const std::string& key_path, int num_threads = 0);

    /**
     * @brief Registers global middleware.
     */
    void use(const Middleware &mw);

    /** @brief Creates a route group with a common prefix. */
    RouteGroup group(const std::string& prefix);

    /**
     * @brief Auto-registers multiple controllers.
     * Requires static void register_routes(App& app) in each controller.
     */
    template<typename... Controllers>
    void register_controllers() {
        (Controllers::register_routes(*this), ...);
    }

    /** @brief Access the internal router. */
    Router& get_router();

    /** @brief Get WebSocket handlers for a specific path. */
    const WebSocketHandlers* get_ws_handler(const std::string& path) const;

    /** @brief Access the application logger. */
    Logger& get_logger();

    /** @brief Returns the internal io_context engine. */
    net::io_context& engine() { return ioc_; }
    boost::asio::awaitable<std::string> handle_request(Request& req, const std::string& client_ip, bool keep_alive);

private:
    void _register_docs();
    boost::asio::awaitable<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler);

    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        using ReturnType = typename function_traits<Func>::return_type;
        using AsyncInfo = extract_async_type<ReturnType>;

        return [this, handler](Request& req, Response& res) -> Async<void> {
            if constexpr (AsyncInfo::is_async && !std::is_void_v<typename AsyncInfo::type>) {
                using InnerT = typename AsyncInfo::type;
                InnerT result = co_await inject_and_call(const_cast<Func&>(handler), services_, req, res);
                
                if constexpr (std::is_convertible_v<InnerT, std::string>) {
                    res.send(result);
                } else if constexpr (std::is_same_v<InnerT, Json>) {
                    // Special handling for our Json wrapper
                    res.json(static_cast<boost::json::value>(result));
                } else {
                    // Generic JSON serialization for Models, Vectors, Maps, etc.
                    res.json(result);
                }
            } else {
                co_await inject_and_call(const_cast<Func&>(handler), services_, req, res);
            }
        };
    }
};

    /**
     * @brief Asynchronously waits for a specified duration.
     * usage: co_await blaze::delay(std::chrono::milliseconds(1000));
     */
    boost::asio::awaitable<void> delay(std::chrono::milliseconds ms);

} // namespace blaze

#endif