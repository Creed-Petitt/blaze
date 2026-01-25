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

    namespace fs = std::filesystem;

    inline bool ends_with(const std::string_view str, const std::string_view suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    inline int hex_value(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }

    inline std::string url_decode(std::string_view input) {
        std::string output;
        output.reserve(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '%' && i + 2 < input.size()) {
                int hi = hex_value(input[i + 1]);
                int lo = hex_value(input[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    output.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                    continue;
                }
            }
            output.push_back(input[i]);
        }
        return output;
    }

    // Thread-safe cache structure for static files
    struct FileCache {
        std::shared_mutex mtx; // Reader-Writer Lock
        std::unordered_map<std::string, std::string> content_map;
        std::unordered_map<std::string, std::string> type_map;
    };

    inline std::string get_mime_type(const std::string& path) {
        if (ends_with(path, ".html")) return "text/html";
        if (ends_with(path, ".css")) return "text/css";
        if (ends_with(path, ".js") || ends_with(path, ".mjs")) return "application/javascript";
        if (ends_with(path, ".json")) return "application/json";
        if (ends_with(path, ".png")) return "image/png";
        if (ends_with(path, ".jpg") || ends_with(path, ".jpeg")) return "image/jpeg";
        if (ends_with(path, ".gif")) return "image/gif";
        if (ends_with(path, ".svg")) return "image/svg+xml";
        if (ends_with(path, ".ico")) return "image/x-icon";
        if (ends_with(path, ".webp")) return "image/webp";
        if (ends_with(path, ".woff2")) return "font/woff2";
        if (ends_with(path, ".woff")) return "font/woff";
        if (ends_with(path, ".ttf")) return "font/ttf";
        return "application/octet-stream"; // Safer default
    }

    inline Middleware cors() {
        return [](Request& req, Response& res, auto next) -> Async<void> {
            res.header("Access-Control-Allow-Origin", "*");
            res.header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.header("Access-Control-Allow-Headers", "Content-Type, Authorization");

            if (req.method == "OPTIONS") {
                res.status(204).send("");
                co_return;
            }

            co_await next();
        };
    }

    inline Middleware cors(const std::string& origin,
                           const std::string& methods = "GET, POST, PUT, DELETE, OPTIONS",
                           const std::string& headers = "Content-Type, Authorization") {
        return [origin, methods, headers](Request& req, Response& res, const auto& next) -> Async<void> {
            res.header("Access-Control-Allow-Origin", origin);
            res.header("Access-Control-Allow-Methods", methods);
            res.header("Access-Control-Allow-Headers", headers);

            if (req.method == "OPTIONS") {
                res.status(204).send("");
                co_return;
            }

            co_await next();
        };
    }

    inline Middleware static_files(const std::string& root_dir, bool serve_index = true) {
        // Shared cache instance
        auto cache = std::make_shared<FileCache>();

        // Resolve absolute path for security checks once
        fs::path abs_root;
        try {
            abs_root = fs::canonical(root_dir);
        } catch (...) {
            abs_root = fs::absolute(root_dir);
        }

        return [abs_root, serve_index, cache](Request& req, Response& res, auto next) -> Async<void> {
            if (req.method != "GET") {
                co_await next();
                co_return;
            }

            std::string decoded_path = url_decode(req.path);

            // 1. Check Cache First (Fast Path - No Syscalls)
            // If it's in the cache, we already validated it was safe when we put it there.
            {
                std::shared_lock<std::shared_mutex> lock(cache->mtx);
                auto it = cache->content_map.find(decoded_path);
                if (it != cache->content_map.end()) {
                    res.header("Content-Type", cache->type_map[decoded_path]);
                    res.send(it->second);
                    co_return;
                }
            }

            // Construct full path
            fs::path requested_path = abs_root / decoded_path.substr(1); // Remove leading slash

            // Security: Canonicalize and ensure it starts with root
            std::error_code ec;
            fs::path canonical_path = fs::canonical(requested_path, ec);

            if (ec) {
                // File doesn't exist or other error, fallback to next middleware
                co_await next();
                co_return;
            }

            // Path Traversal Check: Ensure canonical path starts with abs_root
            std::string p_str = canonical_path.string();
            std::string r_str = abs_root.string();
            if (p_str.compare(0, r_str.length(), r_str) != 0) {
                res.status(403).json({{"error", "Forbidden"}, {"message", "Access Denied"}});
                co_return;
            }

            // Directory Handling (Index)
            if (fs::is_directory(canonical_path, ec) && serve_index) {
                canonical_path /= "index.html";
                if (!fs::exists(canonical_path, ec)) {
                    co_await next();
                    co_return;
                }
            }

            // 2. Read File (Optimized)
            std::string file_real_path = canonical_path.string();
            std::ifstream file(file_real_path, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                co_await next();
                co_return;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            if (size <= 0) {
                res.send("");
                co_return;
            }

            std::string content;
            content.resize(size);
            if (!file.read(content.data(), size)) {
                co_await next();
                co_return;
            }

            std::string content_type = get_mime_type(file_real_path);

            // 3. Write Cache (Exclusive Lock) - Use decoded_path as key to enable Fast Path next time
            if (size < 10 * 1024 * 1024) { // 10MB limit
                std::unique_lock<std::shared_mutex> lock(cache->mtx);
                cache->content_map[decoded_path] = content;
                cache->type_map[decoded_path] = content_type;
            }

            res.header("Content-Type", content_type);
            res.send(content);
            co_return;
        };
    }

    inline Middleware limit_body_size(size_t max_bytes) {
        return [max_bytes](Request& req, Response& res, auto next) -> Async<void> {
            if (req.body.size() > max_bytes) {
                res.status(413).json({
                    {"error", "Request body too large"},
                    {"max_size", max_bytes},
                    {"received_size", req.body.size()}
                });
                co_return;
            }

            co_await next();
        };
    }

    // usage: app.use(middleware::bearer_auth([](std::string_view token) {
    //     return token == "my-secret";
    // }));
    template<typename Validator>
    inline Middleware bearer_auth(Validator validator) {
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

    inline Middleware jwt_auth(const std::string_view secret) {
        std::string secret_str(secret);
        return [secret_str](Request& req, Response& res, auto next) -> Async<void> {
            if (!req.has_header("Authorization")) {
                throw Unauthorized("Missing Authorization header");
            }

            std::string_view auth = req.get_header("Authorization");
            if (auth.substr(0, 7) != "Bearer ") {
                throw Unauthorized("Invalid Authorization scheme (Expected Bearer)");
            }

            std::string_view token = auth.substr(7);
            try {
                auto payload = crypto::jwt_verify(token, secret_str);
                req.set_user(Json(payload));
            } catch (const std::exception& e) {
                throw Unauthorized(std::string("Invalid Token: ") + e.what());
            }

            co_await next();
        };
    }

    inline Middleware rate_limit(int max_requests, int window_seconds) {
        struct ClientState {
            int count;
            std::chrono::steady_clock::time_point window_start;
        };
        
        auto state = std::make_shared<std::pair<std::mutex, std::unordered_map<std::string, ClientState>>>();

        return [max_requests, window_seconds, state](Request& req, Response& res, auto next) -> Async<void> {
            std::string ip = "unknown";
            if (auto ip_ctx = req.get_opt<std::string>("client_ip")) {
                ip = *ip_ctx;
            }

            std::lock_guard lock(state->first);
            auto& clients = state->second;
            auto now = std::chrono::steady_clock::now();

            auto& client = clients[ip];
            
            // Check window reset
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - client.window_start).count();
            if (elapsed > window_seconds) {
                client.count = 0;
                client.window_start = now;
            }

            if (client.count >= max_requests) {
                res.status(429).json({
                    {"error", "Too Many Requests"},
                    {"retry_after_seconds", window_seconds - elapsed}
                });
                co_return;
            }

            client.count++;
            co_await next();
        };
    }

}

#endif
