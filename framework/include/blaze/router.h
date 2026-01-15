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

using Task = boost::asio::awaitable<void>;
using Middleware = std::function<void(Request&, Response&, std::function<void()>)>;
using Handler = std::function<Task(Request&, Response&)>;

struct RouteMatch {
    Handler handler;                                      // The function to call
    std::unordered_map<std::string, std::string> params;  // Extracted params like {"id": "42"}
};

class Router;

// Route grouping for organizing routes with common prefixes
class RouteGroup {
private:
    Router& router_;
    std::string prefix_;

public:
    RouteGroup(Router& router, const std::string& prefix);

    void get(const std::string& path, const Handler &handler) const;
    void post(const std::string& path, const Handler &handler) const;
    void put(const std::string& path, const Handler &handler) const;
    void del(const std::string& path, const Handler &handler) const;

    // Allow nested groups
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
                        const std::vector<std::string>& request_segments,
                        std::unordered_map<std::string, std::string>& params);

    static std::vector<std::string> split(const std::string& str);

public:
    void add_route(const std::string& method, const std::string& path, const Handler &handler);

    std::optional<RouteMatch> match(const std::string& method, const std::string& path) const;
};

} // namespace blaze

#endif
