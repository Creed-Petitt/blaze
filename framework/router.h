#ifndef HTTP_SERVER_ROUTER_H
#define HTTP_SERVER_ROUTER_H

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include "request.h"
#include "response.h"

using Middleware = std::function<void(Request&, Response&, std::function<void()>)>;
using Handler = std::function<void(Request&, Response&)>;

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

    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void del(const std::string& path, Handler handler);

    // Allow nested groups
    RouteGroup group(const std::string& subpath);
};

class Router {
private:

    struct Route {
        std::string method;
        std::string path;
        Handler handler;
    };

    std::vector<Route> routes_;

    bool matches(const std::string& route_path,
                 const std::string& request_path,
                 std::unordered_map<std::string, std::string>& params);

    std::vector<std::string> split(const std::string& str, char delimiter);

public:
    void add_route(const std::string& method, const std::string& path, Handler handler);

    std::optional<RouteMatch> match(const std::string& method, const std::string& path);
};

#endif
