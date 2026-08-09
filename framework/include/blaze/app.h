#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/websocket.h>
#include <blaze/di.h>
#include <blaze/injector.h>
#include <blaze/json.h>
#include <boost/asio.hpp>
#include <functional>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>

namespace net = boost::asio;

namespace blaze {

class ListenerBase;

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
    int shutdown_timeout = 30;               // 30s safety shutdown
    std::string log_path = "stdout";         // Logging destination
    LogLevel log_level = LogLevel::INFO;     // Default log level
    int num_threads = 0;                     // 0 = auto-detect
    std::string server_name = "Blaze/1.0";   // Server header
};


/**
 * @brief The primary entry point for a Blaze application.
 * 
 * The App class manages the internal Boost.Asio engine, application services,
 * and HTTP routing.
 */
class App {
private:
    Router router_;
    std::map<std::string, WebSocketHandlers> ws_routes_;

    // Session tracking for path-based broadcasting
    std::map<std::string, std::vector<std::weak_ptr<WebSocket>>> ws_sessions_;
    std::mutex ws_mtx_;
    
    // Lifecycle Mutex (protects listeners_, signals_)
    std::mutex lifecycle_mtx_;

    void broadcast_raw(const std::string& path, const std::string& payload);

    net::io_context ioc_;
    std::vector<Middleware> middleware_;
    AppConfig config_;
    Services services_;
    std::vector<std::shared_ptr<ListenerBase>> listeners_;
    std::unique_ptr<net::signal_set> signals_;
    std::atomic<bool> stopping_{false};

public:
    App();
    ~App();

    /**
     * @brief Stops the application gracefully.
     * Closes listeners and notifies WebSockets.
     */
    void stop();

    /**
     * @brief Access the application configuration.
     */
    AppConfig& config() { return config_; }
    const AppConfig& get_config() const { return config_; }

    App& log_to(const std::string& path) { config_.log_path = path; return *this; }
    App& log_level(LogLevel level) { config_.log_level = level; Logger::instance().set_level(level); return *this; }
    App& max_body_size(size_t bytes) { config_.max_body_size = bytes; return *this; }
    App& timeout(int seconds) { config_.timeout_seconds = seconds; return *this; }
    App& shutdown_timeout(int seconds) { config_.shutdown_timeout = seconds; return *this; }
    App& num_threads(int n) { config_.num_threads = n; return *this; }
    App& server_name(const std::string& name) { config_.server_name = name; return *this; }

    /**
     * @brief Access app-owned services.
     */
    Services& services() { return services_; }
    const Services& services() const { return services_; }

    /**
     * @brief Registers a GET route.
     * 
     * The handler function supports Request&, Response&, Path<T>, Query<T>,
     * Body<T>, and Context<T>. Use req.service<T>() for app services.
     * 
     * @param path The URL path (e.g., "/users/:id").
     * @param handler The callback function or lambda.
     */
    template<typename Func>
    void get(const std::string& path, Func handler) {
        router_.add_route("GET", path, wrap_handler(handler));
    }

    /** @brief Registers a POST route. */
    template<typename Func>
    void post(const std::string& path, Func handler) {
        router_.add_route("POST", path, wrap_handler(handler));
    }

    /** @brief Registers a PUT route. */
    template<typename Func>
    void put(const std::string& path, Func handler) {
        router_.add_route("PUT", path, wrap_handler(handler));
    }

    /** @brief Registers a DELETE route. */
    template<typename Func>
    void del(const std::string& path, Func handler) {
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
    void _register_ws(const std::string& path, const std::shared_ptr<WebSocket>& ws);

    /** @brief Starts a background task (coroutine) in the event loop. */
    void spawn(Async<void> task);

    /**
     * @brief Starts the HTTP server on the specified port.
     * 
     * @param port The port to listen on.
     * @param num_threads Number of threads for the event loop (0 = auto-detect).
     */
    void listen(int port, int num_threads = 0);

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

    /** @brief Returns the internal io_context engine. */
    net::io_context& engine() { return ioc_; }
    boost::asio::awaitable<Response> handle_request(Request& req, const std::string& client_ip, bool keep_alive);

private:
    void _run_server(int num_threads);
    boost::asio::awaitable<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler);

    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        using ReturnType = typename function_traits<Func>::return_type;
        using AsyncInfo = extract_async_type<ReturnType>;

        return [this, handler](Request& req, Response& res) -> Async<void> {
            if constexpr (AsyncInfo::is_async && !std::is_void_v<typename AsyncInfo::type>) {
                using InnerT = typename AsyncInfo::type;
                InnerT result = co_await inject_and_call(const_cast<Func&>(handler), req, res);
                
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
                co_await inject_and_call(const_cast<Func&>(handler), req, res);
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
