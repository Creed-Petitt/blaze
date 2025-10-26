#include "router.h"
#include <sstream>
#include <vector>

void Router::add_route(const std::string& method, const std::string& path,
                        Handler handler) {
    routes_.push_back({method, path, handler});
}

std::optional<RouteMatch> Router::match(const std::string& method, const std::string& path) {
    for (const auto& route : routes_) {
        if (route.method != method) {
            continue;
        }

        std::unordered_map<std::string, std::string> params;
        if (matches(route.path, path, params)) {
            return RouteMatch{route.handler, params};
        }
    }

    return std::nullopt;
}

bool Router::matches(const std::string& route_path,
                     const std::string& request_path,
                     std::unordered_map<std::string, std::string>& params) {
    std::vector<std::string> route_segments = split(route_path, '/');
    std::vector<std::string> request_segments = split(request_path, '/');

    if (route_segments.size() != request_segments.size()) {
        return false;
    }

    for (size_t i = 0; i < route_segments.size(); i++) {
        const std::string& route_seg = route_segments[i];
        const std::string& request_seg = request_segments[i];

        if (!route_seg.empty() && route_seg[0] == ':') {
            std::string param_name = route_seg.substr(1);
            params[param_name] = request_seg;
        } else {
            if (route_seg != request_seg) {
                return false;
            }
        }
    }

    return true;
}

std::vector<std::string> Router::split(const std::string& str, char delimiter) {
    std::vector<std::string> segments;
    std::stringstream ss(str);
    std::string segment;

    while (std::getline(ss, segment, delimiter)) {
        segments.push_back(segment);
    }

    return segments;
}
