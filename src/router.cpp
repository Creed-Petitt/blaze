#include <blaze/router.h>
#include <blaze/util/string.h>
#include <iostream>
#include <algorithm>

namespace blaze {

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
    return {router_, prefix_ + subpath};
}

void Router::add_route(const std::string& method, const std::string& path, const Handler &handler) {
    routes_.push_back({method, path, split(path), handler});
}

std::optional<RouteMatch> Router::match(std::string_view method, std::string_view path) const {
    // Separate path from query string
    size_t query_pos = path.find('?');
    std::string_view pure_path = path.substr(0, query_pos);

    // Normalize: remove trailing slash if not root
    if (pure_path.size() > 1 && pure_path.back() == '/') {
        pure_path.remove_suffix(1);
    }

    const std::vector<std::string_view> request_segments = split_view(pure_path);

    for (const auto& route : routes_) {
        if (route.method != method) continue;
        if (route.segments.size() != request_segments.size()) continue;

        std::unordered_map<std::string, std::string> params;
        std::vector<std::string> path_values;
        if (matches(route.segments, request_segments, params, path_values)) {
            return RouteMatch{route.handler, std::move(params), std::move(path_values)};
        }
    }

    return std::nullopt;
}

bool Router::matches(const std::vector<std::string>& route_segments,
                     const std::vector<std::string_view>& request_segments,
                     std::unordered_map<std::string, std::string>& params,
                     std::vector<std::string>& path_values) {
    for (size_t i = 0; i < route_segments.size(); i++) {
        const std::string& route_seg = route_segments[i];
        const std::string_view& request_seg = request_segments[i];

        if (!route_seg.empty() && route_seg[0] == ':') {
            std::string param_name = route_seg.substr(1);
            std::string param_value = util::url_decode(request_seg);
            params[param_name] = param_value;
            path_values.push_back(param_value);
        } else {
            if (route_seg != request_seg) {
                return false;
            }
        }
    }
    return true;
}

std::vector<std::string> Router::split(const std::string& str) {
    auto views = split_view(str);
    std::vector<std::string> segments;
    segments.reserve(views.size());
    for (auto v : views) {
        segments.emplace_back(v);
    }
    return segments;
}

std::vector<std::string_view> Router::split_view(std::string_view str) {
    std::vector<std::string_view> segments;
    if (str.empty() || str == "/") {
        segments.emplace_back("");
        return segments;
    }

    size_t start = 0;
    while (start < str.size()) {
        if (str[start] == '/') {
            start++;
            continue;
        }
        size_t end = str.find('/', start);
        if (end == std::string_view::npos) {
            end = str.size();
        }
        
        segments.push_back(str.substr(start, end - start));
        start = end;
    }

    if (segments.empty()) {
        segments.emplace_back("");
    }

    return segments;
}

} // namespace blaze
