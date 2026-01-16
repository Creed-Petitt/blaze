#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <blaze/json.h>

namespace blaze {

struct Request {
    std::string_view method;
    std::string_view path;
    std::string_view body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;

    // Zero-copy reference to Beast headers
    const boost::beast::http::header<true, boost::beast::http::fields>* fields_ = nullptr;

    void set_target(std::string_view target);
    void set_fields(const boost::beast::http::header<true, boost::beast::http::fields>& fields);

    // Returns parsed JSON body wrapper
    blaze::Json json() const;

    // Helper methods for safe parameter and header access
    std::string get_query(const std::string& key, const std::string& default_val = "") const;
    int get_query_int(const std::string& key, int default_val = 0) const;
    
    std::string_view get_header(std::string_view key) const;
    bool has_header(std::string_view key) const;
    
    std::optional<int> get_param_int(const std::string& key) const;
};

} // namespace blaze

#endif