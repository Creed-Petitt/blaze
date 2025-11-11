#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <string>
#include <unordered_map>
#include <optional>
#include "../json.hpp"

using json = nlohmann::json;

class Request {
public:
    explicit Request(std::string& raw_http);

    std::string method;
    std::string path;
    std::string http_version;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;
    std::string body;

    nlohmann::json json() const;

    // Helper methods for safe parameter and header access
    std::string get_query(const std::string& key, const std::string& default_val = "") const;
    int get_query_int(const std::string& key, int default_val = 0) const;
    std::string get_header(const std::string& key, const std::string& default_val = "") const;
    bool has_header(const std::string& key) const;
    std::optional<int> get_param_int(const std::string& key) const;

    // Static helper to extract Content-Length before full parsing (shared with HttpServer)
    static std::optional<size_t> extract_content_length(
        const std::string& buffer,
        size_t headers_end,
        size_t max_size = 100 * 1024 * 1024  // 100 MB default
    );

private:
    static size_t parse_content_length_value(const std::string& value);
};

#endif