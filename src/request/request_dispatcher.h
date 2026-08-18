#pragma once

#include <blaze/config.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/router.h>

#include <string>
#include <vector>

namespace blaze {

class RequestDispatcher {
    Router& router_;
    std::vector<Middleware>& middleware_;
    const Config& config_;

public:
    RequestDispatcher(Router& router, std::vector<Middleware>& middleware, const Config& config);

    Async<Response> handle(Request& req, const std::string& client_ip, bool keep_alive);

private:
    Async<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler);
};

} // namespace blaze
