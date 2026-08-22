#pragma once

#include <blaze/router.h>
#include <blaze/logger.h>
#include <blaze/injector.h>
#include <blaze/json.h>
#include <blaze/config.h>

#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>

namespace blaze {

class Server;
class Dispatcher;

/**
 * @brief Represents the lifecycle state of a Blaze application.
 */
enum class AppState {
    Configuring,
    Running,
    Stopping,
    Stopped
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
    const Config config_;
    std::vector<Middleware> middleware_;
    std::unique_ptr<Dispatcher> dispatcher_;
    std::unique_ptr<Server> server_;
    mutable AppState state_{AppState::Configuring};

public:
    App();

    explicit App(Config config);

    ~App();

    /**
     * @brief Stops the application.
     * Closes the listener and stops the event loop.
     */
    void stop() const;

    /**
     * @brief Access the current lifecycle state.
     */
    AppState state() const { return state_; }

    /**
     * @brief Access the application configuration.
     */
    const Config& config() const { return config_; }


    /**
     * @brief Registers a GET route.
     *
     * Body<T>, and Context<T>.
     * The handler function supports Request&, Response&, Path<T>, Query<T>,
     *
     * @param path The URL path (e.g., "/users/:id").
     * @param handler The callback function or lambda.
     */
    template<typename Func>
    void get(const std::string& path, Func handler) {
        ensure_configuring("Cannot register GET route");
        router_.add_route("GET", path, wrap_handler(handler));
    }

    /** @brief Registers a POST route. */
    template<typename Func>
    void post(const std::string& path, Func handler) {
        ensure_configuring("Cannot register POST route");
        router_.add_route("POST", path, wrap_handler(handler));
    }

    /** @brief Registers a PUT route. */
    template<typename Func>
    void put(const std::string& path, Func handler) {
        ensure_configuring("Cannot register PUT route");
        router_.add_route("PUT", path, wrap_handler(handler));
    }

    /** @brief Registers a DELETE route. */
    template<typename Func>
    void del(const std::string& path, Func handler) {
        ensure_configuring("Cannot register DELETE route");
        router_.add_route("DELETE", path, wrap_handler(handler));
    }

    /** @brief Starts a background task (coroutine) in the event loop. */
    void spawn(Async<void> task) const;

    /**
     * @brief Starts the HTTP server on the specified port.
     *
     * @param port The port to listen on.
     * @param num_threads Number of threads for the event loop (0 = auto-detect).
     */
    void listen(int port) const;

    /**
     * @brief Registers global middleware.
     */
    void use(const Middleware &mw);

    /**
     * @brief Auto-registers multiple controllers.
     * Requires static void register_routes(App& app) in each controller.
     */
    template<typename... Controllers>
    void register_controllers() {
        ensure_configuring("Cannot register controllers");
        (Controllers::register_routes(*this), ...);
    }

    /** @brief Access the internal router. */
    const Router& get_router() const;

private:
    void ensure_configuring(const std::string& action) const {
        if (state_ != AppState::Configuring) {
            throw std::logic_error(action + ": cannot mutate app once it has started listening.");
        }
    }

    void check_configuring(const std::string& action) const {
        ensure_configuring(action);
    }

    // Takes lambda and converts it into a standard (Request, Response) handler
    template<typename Func>
    Handler wrap_handler(Func handler) {
        using ReturnType = function_traits<Func>::return_type;
        using AsyncInfo = async_result<ReturnType>;

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
