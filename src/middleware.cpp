#include <blaze/middleware.h>
#include <blaze/util/string.h>
#include <filesystem>
#include <fstream>
#include <shared_mutex>

namespace blaze::middleware {

namespace fs = std::filesystem;

// Thread-safe cache structure for static files
struct FileCache {
    std::shared_mutex mtx; // Reader-Writer Lock
    std::unordered_map<std::string, std::string> type_map;
    // We no longer cache content here since we use file_body for streaming,
    // but we can cache MIME types or small file metadata if needed.
};

static std::string get_mime_type(const std::string& path) {
    auto ends_with = [](const std::string_view str, const std::string_view suffix) {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if (ends_with(path, ".html")) return "text/html";
    if (ends_with(path, ".css")) return "text/css";
    if (ends_with(path, ".js") || ends_with(path, ".mjs")) return "application/javascript";
    if (ends_with(path, ".json")) return "application/json";
    if (ends_with(path, ".png")) return "image/png";
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg")) return "image/jpeg";
    if (ends_with(path, ".pdf")) return "application/pdf";
    if (ends_with(path, ".gif")) return "image/gif";
    if (ends_with(path, ".svg")) return "image/svg+xml";
    if (ends_with(path, ".ico")) return "image/x-icon";
    if (ends_with(path, ".webp")) return "image/webp";
    if (ends_with(path, ".woff2")) return "font/woff2";
    if (ends_with(path, ".woff")) return "font/woff";
    if (ends_with(path, ".ttf")) return "font/ttf";
    if (ends_with(path, ".txt")) return "text/plain";
    return "application/octet-stream";
}

Middleware cors() {
    return [](const Request& req, Response& res, auto next) -> Async<void> {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

        if (req.method == "OPTIONS") {
            res.status(204).send("");
            co_return;
        }

        co_await next();
    };
}

Middleware cors(const std::string& origin, const std::string& methods, const std::string& headers) {
    return [origin, methods, headers](const Request& req, Response& res, const auto& next) -> Async<void> {
        res.set_header("Access-Control-Allow-Origin", origin);
        res.set_header("Access-Control-Allow-Methods", methods);
        res.set_header("Access-Control-Allow-Headers", headers);

        if (req.method == "OPTIONS") {
            res.status(204).send("");
            co_return;
        }

        co_await next();
    };
}

Middleware static_files(const std::string& root_dir, bool serve_index) {
    auto cache = std::make_shared<FileCache>();
    fs::path abs_root;
    try {
        abs_root = fs::canonical(root_dir);
    } catch (...) {
        abs_root = fs::absolute(root_dir);
    }

    return [abs_root, serve_index, cache](const Request& req, Response& res, auto next) -> Async<void> {
        if (req.method != "GET") {
            co_await next();
            co_return;
        }

        const std::string decoded_path = util::url_decode(req.path);
        const fs::path requested_path = abs_root / decoded_path.substr(1);

        std::error_code ec;
        fs::path canonical_path = fs::canonical(requested_path, ec);

        if (ec) {
            co_await next();
            co_return;
        }

        const std::string p_str = canonical_path.string();
        const std::string r_str = abs_root.string();
        if (p_str.compare(0, r_str.length(), r_str) != 0) {
            res.status(403).json(Json{{"error", "Forbidden"}, {"message", "Access Denied"}});
            co_return;
        }

        if (fs::is_directory(canonical_path, ec) && serve_index) {
            canonical_path /= "index.html";
            if (!fs::exists(canonical_path, ec)) {
                co_await next();
                co_return;
            }
        }

        const std::string file_real_path = canonical_path.string();
        
        // Fast path for MIME type
        std::string content_type;
        {
            std::shared_lock lock(cache->mtx);
            const auto it = cache->type_map.find(file_real_path);
            if (it != cache->type_map.end()) {
                content_type = it->second;
            }
        }

        if (content_type.empty()) {
            content_type = get_mime_type(file_real_path);
            std::unique_lock lock(cache->mtx);
            cache->type_map[file_real_path] = content_type;
        }

        res.set_header("Content-Type", content_type);
        res.file(file_real_path); // ZERO-COPY STREAMING!
        co_return;
    };
}

Middleware limit_body_size(size_t max_bytes) {
    return [max_bytes](const Request& req, Response& res, auto next) -> Async<void> {
        if (req.body.size() > max_bytes) {
            res.status(413).json(Json{
                {"error", "Request body too large"},
                {"max_size", max_bytes},
                {"received_size", req.body.size()}
            });
            co_return;
        }
        co_await next();
    };
}

Middleware rate_limit(int max_requests, int window_seconds) {
    struct ClientState {
        int count{};
        std::chrono::steady_clock::time_point window_start;
    };
    
    auto state = std::make_shared<std::pair<std::mutex, std::unordered_map<std::string, ClientState>>>();

    return [max_requests, window_seconds, state](const Request& req, Response& res, auto next) -> Async<void> {
        std::string ip = "unknown";
        if (const auto ip_ctx = req.get_opt<std::string>("client_ip")) {
            ip = *ip_ctx;
        }

        {
            std::lock_guard lock(state->first);
            auto& clients = state->second;
            const auto now = std::chrono::steady_clock::now();
            auto& [count, window_start] = clients[ip];

            const auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(now - window_start)
                    .count();
            if (elapsed > window_seconds) {
                count = 0;
                window_start = now;
            }

            if (count >= max_requests) {
                res.status(429).json(Json{
                    {"error", "Too Many Requests"},
                    {"retry_after_seconds", window_seconds - elapsed}
                });
                co_return;
            }
            count++;
        }
        co_await next();
    };
}

} // namespace blaze::middleware
