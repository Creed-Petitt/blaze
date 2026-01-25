#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <boost/json.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <any>
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

    void set_user(blaze::Json user) { user_context_ = std::move(user); }
    
    // Returns the user object directly. Throws Unauthorized if not authenticated.
    const blaze::Json& user() const {
        if (!user_context_.has_value()) {
            throw blaze::Unauthorized("User not authenticated");
        }
        return *user_context_;
    }

    bool is_authenticated() const { return user_context_.has_value(); }

    std::string cookie(const std::string& name) const;

    template<typename T>
    void set(const std::string& key, T&& value) {
        context_[key] = std::make_any<T>(std::forward<T>(value));
    }

    template<typename T>
    T get(const std::string& key) const {
        auto it = context_.find(key);
        if (it == context_.end()) {
            throw std::runtime_error("Key not found in request context: " + key);
        }
        return std::any_cast<T>(it->second);
    }

    template<typename T>
    std::optional<T> get_opt(const std::string& key) const {
        const auto it = context_.find(key);
        if (it == context_.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        } catch (...) {
            return std::nullopt;
        }
    }

private:
    std::optional<blaze::Json> user_context_;
    std::unordered_map<std::string, std::any> context_;
};

} // namespace blaze

#endif