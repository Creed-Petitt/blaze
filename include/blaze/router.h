#pragma once

#include <blaze/handler.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace blaze {

struct MatchedRoute {
    Handler handler;
    std::unordered_map<std::string, std::string> params;
    std::vector<std::string> path_values;
};

class Router {
public:
    Router();
    ~Router();

    Router(Router&&) noexcept;
    Router& operator=(Router&&) noexcept;

    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    void add_route(std::string_view method, const std::string& path, Handler handler) const;

    std::optional<MatchedRoute> match(std::string_view method, std::string_view path) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace blaze
