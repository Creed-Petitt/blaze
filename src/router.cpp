#include <blaze/router.h>
#include <blaze/util/string.h>

#include <algorithm>
#include <utility>

namespace blaze {

namespace {

struct RouteEntry {
    std::string method;
    std::string path;
    std::vector<std::string> segments;
    Handler handler;
};

std::string_view path_without_query(std::string_view target) {
    const size_t query_pos = target.find('?');
    auto path = target.substr(0, query_pos);

    if (path.size() > 1 && path.back() == '/') {
        path.remove_suffix(1);
    }

    return path;
}

std::vector<std::string_view> split_view(std::string_view path) {
    std::vector<std::string_view> segments;
    if (path.empty() || path == "/") {
        segments.emplace_back("");
        return segments;
    }

    size_t start = 0;
    while (start < path.size()) {
        if (path[start] == '/') {
            ++start;
            continue;
        }

        size_t end = path.find('/', start);
        if (end == std::string_view::npos) {
            end = path.size();
        }

        segments.push_back(path.substr(start, end - start));
        start = end;
    }

    if (segments.empty()) {
        segments.emplace_back("");
    }

    return segments;
}

std::vector<std::string> split(const std::string_view path) {
    const auto views = split_view(path);
    std::vector<std::string> segments;
    segments.reserve(views.size());

    for (auto segment : views) {
        segments.emplace_back(segment);
    }

    return segments;
}

bool match_segments(
    const std::vector<std::string>& route_segments,
    const std::vector<std::string_view>& request_segments,
    std::unordered_map<std::string, std::string>& params,
    std::vector<std::string>& path_values)
{
    for (size_t i = 0; i < route_segments.size(); ++i) {
        const std::string& route_segment = route_segments[i];
        const std::string_view request_segment = request_segments[i];

        if (!route_segment.empty() && route_segment.front() == ':') {
            const std::string name = route_segment.substr(1);
            std::string value = util::url_decode(request_segment);
            params[name] = value;
            path_values.push_back(std::move(value));
            continue;
        }

        if (route_segment != request_segment) {
            return false;
        }
    }

    return true;
}

} // namespace

struct Router::Impl {
    std::vector<RouteEntry> routes;
};

Router::Router() : impl_(std::make_unique<Impl>()) {}

Router::~Router() = default;

Router::Router(Router&&) noexcept = default;

Router& Router::operator=(Router&&) noexcept = default;

void Router::add_route(const std::string_view method, const std::string& path, Handler handler) const
{
    impl_->routes.push_back({
        std::string(method),
        path,
        split(path),
        std::move(handler)
    });
}

std::optional<MatchedRoute> Router::match(
    const std::string_view method,
    const std::string_view path) const
{
    const auto request_path = path_without_query(path);
    const auto request_segments = split_view(request_path);

    for (const auto& route : impl_->routes) {
        if (route.method != method) continue;
        if (route.segments.size() != request_segments.size()) continue;

        std::unordered_map<std::string, std::string> params;
        std::vector<std::string> path_values;
        if (match_segments(route.segments, request_segments, params, path_values)) {
            return MatchedRoute{route.handler, std::move(params), std::move(path_values)};
        }
    }

    return std::nullopt;
}

} // namespace blaze
