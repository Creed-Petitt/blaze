#ifndef HTTP_SERVER_ROUTER_H
#define HTTP_SERVER_ROUTER_H

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include <blaze/request.h>
#include <blaze/response.h>
#include <boost/asio/awaitable.hpp>

namespace blaze {

template <typename T = void>
using Async = boost::asio::awaitable<T>;

using Task = Async<void>;

using Next = std::function<Task()>;
using Middleware = std::function<Task(Request&, Response&, Next)>;
using Handler = std::function<Task(Request&, Response&)>;

struct RouteMatch {
    Handler handler;                                      // The function to call
    std::unordered_map<std::string, std::string> params;  // Extracted params like {"id": "42"}
};

class Router;

/**
 * @brief A helper class for grouping routes under a common path prefix.
 */
class RouteGroup {
private:
    Router& router_;
    std::string prefix_;

public:
    RouteGroup(Router& router, const std::string& prefix);

    /** @brief Registers a GET route within this group. */
    void get(const std::string& path, const Handler &handler) const;
    
    /** @brief Registers a POST route within this group. */
    void post(const std::string& path, const Handler &handler) const;
    
    /** @brief Registers a PUT route within this group. */
    void put(const std::string& path, const Handler &handler) const;
    
    /** @brief Registers a DELETE route within this group. */
    void del(const std::string& path, const Handler &handler) const;

    /**
     * @brief Creates a nested route group.
     * 
     * @param subpath The prefix to append to the current group's prefix.
     */
    RouteGroup group(const std::string& subpath) const;
};

class Router {
private:

    struct Route {
        std::string method;
        std::string path;
        std::vector<std::string> segments;
        Handler handler;
    };

    std::vector<Route> routes_;

    static bool matches(const std::vector<std::string>& route_segments,
                        const std::vector<std::string_view>& request_segments,
                        std::unordered_map<std::string, std::string>& params);

    static std::vector<std::string> split(const std::string& str);
    static std::vector<std::string_view> split_view(std::string_view str);

public:
    void add_route(const std::string& method, const std::string& path, const Handler &handler);

    std::optional<RouteMatch> match(std::string_view method, std::string_view path) const;
};

} // namespace blaze

#endif
