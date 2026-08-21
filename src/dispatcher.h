#pragma once

#include <blaze/config.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/router.h>

#include <string>
#include <vector>

namespace blaze {

class Dispatcher {
    const Router& router_;
    const std::vector<Middleware>& middleware_;
    const Config& config_;

public:
    Dispatcher(const Router& router, const std::vector<Middleware>& middleware, const Config& config);

    Async<Response> handle(Request& req, const std::string& client_ip, bool keep_alive) const ;

private:
    Async<void> run_middleware(size_t index, Request& req, Response& res, const Handler& final_handler) const;
};

} // namespace blaze
