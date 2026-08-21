#pragma once

#include <blaze/handler.h>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace blaze {

struct MatchedRoute {
    Handler handler;
    std::unordered_map<std::string, std::string> params;
    std::vector<std::string> path_values;
};

class Router {
public:
    struct RouteEntry {
        std::string method;
        std::string path;
        std::vector<std::string> segments;
        Handler handler;
    };

    Router() = default;
    ~Router() = default;

    Router(Router&&) noexcept = default;
    Router& operator=(Router&&) noexcept = default;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    void add_route(std::string_view method, const std::string& path, Handler handler);

    std::optional<MatchedRoute> match(std::string_view method, std::string_view path) const;

    const std::vector<RouteEntry>& routes() const noexcept { return routes_; }

private:
    std::vector<RouteEntry> routes_;
};

} // namespace blaze
