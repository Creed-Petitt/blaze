#include <blaze/router.h>
#include <vector>

namespace blaze {

// RouteGroup implementation
RouteGroup::RouteGroup(Router& router, const std::string& prefix)
    : router_(router), prefix_(prefix) {}

void RouteGroup::get(const std::string& path, const Handler &handler) const {
    router_.add_route("GET", prefix_ + path, handler);
}

void RouteGroup::post(const std::string& path, const Handler &handler) const {
    router_.add_route("POST", prefix_ + path, handler);
}

void RouteGroup::put(const std::string& path, const Handler &handler) const {
    router_.add_route("PUT", prefix_ + path, handler);
}

void RouteGroup::del(const std::string& path, const Handler &handler) const {
    router_.add_route("DELETE", prefix_ + path, handler);
}

RouteGroup RouteGroup::group(const std::string& subpath) const {
    return RouteGroup(router_, prefix_ + subpath);
}

// Router implementation
void Router::add_route(const std::string& method, const std::string& path,
                        const Handler &handler) {
    routes_.push_back({method, path, split(path), handler});
}

std::optional<RouteMatch> Router::match(std::string_view method, std::string_view path) const {
    if (path.size() > 1 && path.back() == '/') {
        path.remove_suffix(1);
    }

    const std::vector<std::string_view> request_segments = split_view(path);

    for (const auto& route : routes_) {
        if (route.method != method) {
            continue;
        }

        if (route.segments.size() != request_segments.size()) {
            continue;
        }

        std::unordered_map<std::string, std::string> params;
        if (matches(route.segments, request_segments, params)) {
            return RouteMatch{route.handler, params};
        }
    }

    return std::nullopt;
}

bool Router::matches(const std::vector<std::string>& route_segments,
                     const std::vector<std::string_view>& request_segments,
                     std::unordered_map<std::string, std::string>& params) {
    for (size_t i = 0; i < route_segments.size(); i++) {
        const std::string& route_seg = route_segments[i];
        const std::string_view& request_seg = request_segments[i];

        if (!route_seg.empty() && route_seg[0] == ':') {
            std::string param_name = route_seg.substr(1);
            params[param_name] = std::string(request_seg);
        } else {
            if (route_seg != request_seg) {
                return false;
            }
        }
    }

    return true;
}

std::vector<std::string> Router::split(const std::string& str) {
    std::vector<std::string> segments;
    size_t start = 0;
    while (start <= str.size()) {
        size_t end = str.find('/', start);
        if (end == std::string::npos) {
            end = str.size();
        }
        segments.emplace_back(str.substr(start, end - start));
        if (end == str.size()) {
            break;
        }
        start = end + 1;
    }

    if (segments.empty()) {
        segments.emplace_back();
    }

    return segments;
}

std::vector<std::string_view> Router::split_view(std::string_view str) {
    std::vector<std::string_view> segments;
    size_t start = 0;
    while (start <= str.size()) {
        size_t end = str.find('/', start);
        if (end == std::string_view::npos) {
            end = str.size();
        }
        segments.emplace_back(str.substr(start, end - start));
        if (end == str.size()) {
            break;
        }
        start = end + 1;
    }

    if (segments.empty()) {
        segments.emplace_back();
    }

    return segments;
}

} // namespace blaze