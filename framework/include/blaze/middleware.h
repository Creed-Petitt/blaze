#ifndef HTTP_SERVER_MIDDLEWARE_H
#define HTTP_SERVER_MIDDLEWARE_H

#include <blaze/router.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <sstream>

namespace middleware {

    inline bool ends_with(const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    inline int hex_value(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }

    inline std::string url_decode(const std::string& input) {
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

    inline Middleware cors() {
        return [](Request& req, Response& res, auto next) {
            res.header("Access-Control-Allow-Origin", "*");
            res.header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.header("Access-Control-Allow-Headers", "Content-Type, Authorization");

            if (req.method == "OPTIONS") {
                res.status(204).send("");
                return;
            }

            next();
        };
    }

    inline Middleware cors(const std::string& origin,
                          const std::string& methods = "GET, POST, PUT, DELETE, OPTIONS",
                          const std::string& headers = "Content-Type, Authorization") {
        return [origin, methods, headers](Request& req, Response& res, const auto& next) {
            res.header("Access-Control-Allow-Origin", origin);
            res.header("Access-Control-Allow-Methods", methods);
            res.header("Access-Control-Allow-Headers", headers);

            if (req.method == "OPTIONS") {
                res.status(204).send("");
                return;
            }

            next();
        };
    }

    inline Middleware static_files(const std::string& directory, bool serve_index = true) {
        return [directory, serve_index](Request& req, Response& res, auto next) {
            if (req.method != "GET") {
                next();
                return;
            }

            std::string decoded_path = url_decode(req.path);

            if (decoded_path.find("..") != std::string::npos || decoded_path.find('\\') != std::string::npos) {
                res.status(403).json({
                    {"error", "Forbidden"},
                    {"message", "Path traversal detected"}
                });
                return;
            }

            std::string file_path = directory + decoded_path;

            struct stat stat_buf;
            if (stat(file_path.c_str(), &stat_buf) != 0 || !S_ISREG(stat_buf.st_mode)) {
                // If path is a directory and serve_index is enabled, try index.html
                if (serve_index && stat(file_path.c_str(), &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode)) {
                    std::string index_path = file_path;
                    if (!index_path.empty() && index_path.back() != '/') {
                        index_path += '/';
                    }
                    index_path += "index.html";

                    if (stat(index_path.c_str(), &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
                        file_path = index_path;
                    } else {
                        next();
                        return;
                    }
                } else {
                    next();
                    return;
                }
            }

            std::ifstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                next();
                return;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            std::string content_type = "text/plain";
            if (ends_with(file_path, ".html")) content_type = "text/html";
            else if (ends_with(file_path, ".css")) content_type = "text/css";
            else if (ends_with(file_path, ".js")) content_type = "application/javascript";
            else if (ends_with(file_path, ".json")) content_type = "application/json";
            else if (ends_with(file_path, ".png")) content_type = "image/png";
            else if (ends_with(file_path, ".jpg") || ends_with(file_path, ".jpeg")) content_type = "image/jpeg";
            else if (ends_with(file_path, ".gif")) content_type = "image/gif";
            else if (ends_with(file_path, ".svg")) content_type = "image/svg+xml";

            res.header("Content-Type", content_type);
            res.send(content);

        };
    }

    inline Middleware limit_body_size(size_t max_bytes) {
        return [max_bytes](Request& req, Response& res, auto next) {
            if (req.body.size() > max_bytes) {
                res.status(413).json({
                    {"error", "Request body too large"},
                    {"max_size", max_bytes},
                    {"received_size", req.body.size()}
                });
                return;
            }

            next();
        };
    }

}

#endif
