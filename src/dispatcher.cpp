#include "dispatcher.h"

#include <blaze/exceptions.h>
#include <blaze/logger.h>

#include <chrono>

namespace blaze {

Dispatcher::Dispatcher(
    const Router& router,
    const std::vector<Middleware>& middleware,
    const Config& config) : router_(router), middleware_(middleware), config_(config) {}

Async<void> Dispatcher::run_middleware(
    size_t index,
    Request& req,
    Response& res,
    const Handler& final_handler) const {
    if (index < middleware_.size()) {
        const auto& mw = middleware_[index];
        co_await mw(req, res, [this, index, &req, &res, &final_handler]() -> Async<void> {
            co_await run_middleware(index + 1, req, res, final_handler);
        });
    } else {
        co_await final_handler(req, res);
    }
}

Async<Response> Dispatcher::handle(Request& req, const std::string& client_ip, const bool keep_alive) const {
    const auto start_time = std::chrono::steady_clock::now();
    Response res;
    int status_code{};

    try {
        req.set("client_ip", client_ip);
        const auto match = router_.match(req.method, req.path);

        Handler handler;
        if (match.has_value()) {
            req.params = match->params;
            req.path_values = match->path_values;
            handler = match->handler;
        } else {
            handler = [](Request&, Response& resp) -> Async<void> {
                resp.status(404).send("404 Not Found\n");
                co_return;
            };
        }

        co_await run_middleware(0, req, res, handler);
        status_code = res.get_status();
    } catch (const HttpError& e) {
        res.status(e.status()).json({
            {"error", "HTTP Error"},
            {"message", e.what()}
        });
        status_code = e.status();
    } catch (const std::exception& e) {
        res.status(500).json({
            {"error", "Internal Server Error"},
            {"message", e.what()}
        });
        status_code = 500;
        Logger::instance().log_error(std::string("Exception in handle_request: ") + e.what());
    }

    res.header("Connection", keep_alive ? "keep-alive" : "close");
    res.header("Server", config_.server_name);

    const auto end_time = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    Logger::instance().log_access(client_ip, req.method, req.path, status_code, duration);

    co_return res;
}

} // namespace blaze
