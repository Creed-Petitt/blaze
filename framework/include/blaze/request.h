#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <json.hpp>

namespace blaze {

using json = nlohmann::json;

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;

    void set_target(std::string_view target);
    void add_header(std::string_view key, std::string_view value);

    nlohmann::json json() const;

    // Helper methods for safe parameter and header access
    std::string get_query(const std::string& key, const std::string& default_val = "") const;
    int get_query_int(const std::string& key, int default_val = 0) const;
    std::string get_header(const std::string& key, const std::string& default_val = "") const;
    bool has_header(const std::string& key) const;
    std::optional<int> get_param_int(const std::string& key) const;
};

} // namespace blaze

#endif