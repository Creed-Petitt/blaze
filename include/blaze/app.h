#pragma once

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/websocket.h>
#include <blaze/websocket_registry.h>
#include <blaze/di.h>
#include <blaze/injector.h>
#include <blaze/json.h>
#include <blaze/config.h>

#include <functional>
#include <vector>
#include <memory>

namespace blaze {

class Server;
class RequestDispatcher;

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

/**
 * @brief The primary entry point for a Blaze application.
 * 
 * The App class manages the internal Boost.Asio engine, application services,
 * and HTTP routing.
 */
class App {
private:
    Router router_;
    WebSocketRegistry websockets_;

    void broadcast_raw(const std::string& path, const std::string& payload);

    std::vector<Middleware> middleware_;
    const Config config_;
    std::unique_ptr<RequestDispatcher> dispatcher_;
    std::unique_ptr<Server> server_;

public:
    App();

    explicit App(Config config);

    ~App();

    /**
     * @brief Stops the application gracefully.
     * Closes listeners and notifies WebSockets.
     */
    void stop();

    /**
     * @brief Access the application configuration.
     */
    const Config& config() const { return config_; }


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

    /** @brief Starts a background task (coroutine) in the event loop. */
    void spawn(Async<void> task) const;

    /**
     * @brief Starts the HTTP server on the specified port.
     *
     * @param port The port to listen on.
     * @param num_threads Number of threads for the event loop (0 = auto-detect).
     */
    void listen(int port, int num_threads = 0) const;

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

    WebSocketRegistry& websockets() { return websockets_; }

private:
    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        using ReturnType = function_traits<Func>::return_type;
        using AsyncInfo = extract_async_type<ReturnType>;

        return [this, handler](Request& req, Response& res) -> Async<void> {
            if constexpr (AsyncInfo::is_async && !std::is_void_v<typename AsyncInfo::type>) {
                using InnerT = AsyncInfo::type;
                InnerT result = co_await inject_and_call(const_cast<Func&>(handler), req, res);

                if constexpr (std::is_convertible_v<InnerT, std::string>) {
                    res.send(result);
                } else if constexpr (std::is_same_v<InnerT, Json>) {
                    // Special handling for our JSON wrapper
                    res.json(result);
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

} // namespace blaze
