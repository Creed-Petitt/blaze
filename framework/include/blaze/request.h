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
#include <blaze/exceptions.h>

namespace blaze {

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;

    // Owned copy of Beast headers
    boost::beast::http::fields headers;

    void set_target(std::string_view target);
    void set_fields(boost::beast::http::fields&& fields);

    // Returns parsed JSON body wrapper
    blaze::Json json() const;


    // Typed JSON Extraction
    // Automatically converts request body to T (std::vector, std::map, etc.)
    template<typename T>
    T json() const {
        try {
            auto val = boost::json::parse(body);
            return boost::json::value_to<T>(val);
        } catch (const std::exception& e) {
            throw BadRequest(std::string("Invalid JSON body: ") + e.what());
        }
    }

    // Helper methods for safe parameter and header access
    std::string get_query(const std::string& key, const std::string& default_val = "") const;
    int get_query_int(const std::string& key, int default_val = 0) const;
    
    std::string_view get_header(std::string_view key) const;
    bool has_header(std::string_view key) const;
    
    std::optional<int> get_param_int(const std::string& key) const;
};

} // namespace blaze

#endif