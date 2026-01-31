#ifndef HTTP_SERVER_MIDDLEWARE_H
#define HTTP_SERVER_MIDDLEWARE_H

#include <blaze/router.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/crypto.h>
#include <blaze/exceptions.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <filesystem>

namespace blaze::middleware
{
    /** @brief Enables Cross-Origin Resource Sharing (CORS) with default settings (Allow All). */
    Middleware cors();

    /** @brief Enables CORS with custom settings. */
    Middleware cors(const std::string& origin,
                   const std::string& methods = "GET, POST, PUT, DELETE, OPTIONS",
                   const std::string& headers = "Content-Type, Authorization");

    /** @brief Serves static files from a directory. Supports streaming via file_body. */
    Middleware static_files(const std::string& root_dir, bool serve_index = true);

    /** @brief Limits the size of the request body. */
    Middleware limit_body_size(size_t max_bytes);

    /** @brief Bearer Token Authentication middleware. */
    template<typename Validator>
    Middleware bearer_auth(Validator validator) {
        return [validator](Request& req, Response& res, auto next) -> Async<void> {
            if (!req.has_header("Authorization")) {
                res.status(401).json({{"error", "Unauthorized"}, {"message", "Missing Authorization header"}});
                co_return;
            }

            const std::string_view auth = req.get_header("Authorization");
            if (auth.substr(0, 7) != "Bearer ") {
                res.status(401).json({{"error", "Unauthorized"}, {"message", "Invalid Authorization scheme"}});
                co_return;
            }

            std::string_view token = auth.substr(7);
            if (!validator(token)) {
                res.status(403).json({{"error", "Forbidden"}, {"message", "Invalid Token"}});
                co_return;
            }

            co_await next();
        };
    }

    /** @brief JWT Authentication middleware. */
    Middleware jwt_auth(const std::string_view secret);

    /** @brief Basic Rate Limiting middleware. */
    Middleware rate_limit(int max_requests, int window_seconds);
}

#endif
