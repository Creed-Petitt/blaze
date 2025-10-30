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

};

#endif