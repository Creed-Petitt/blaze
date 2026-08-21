#pragma once

#include <blaze/header.h>
#include <blaze/json.h>

#include <any>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace blaze {

class Request {
public:

    // Public HTTP data members
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> query;
    std::vector<std::string> path_values;

    // Target and headers
    void set_target(std::string_view target);
    void set_header(std::string_view key, std::string_view value);
    void add_header(std::string_view key, std::string_view value);
    std::string_view get_header(std::string_view key) const;
    bool has_header(std::string_view key) const;
    const std::vector<Header>& headers() const { return headers_; }

    static std::string url_decode(std::string_view str);

    // Query and param helpers
    std::string get_query(const std::string& key, const std::string& default_val = "") const;
    int get_query_int(const std::string& key, int default_val = 0) const;
    std::optional<int> get_param_int(const std::string& key) const;

    // Body parsing
    Json json() const;

    template<typename T>
    T json() const {
        return this->json().template as<T>();
    }

    // Context bag for middleware
    template<typename T>
    void set(const std::string& key, T&& value) {
        context_[key] = std::make_any<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    template<typename T>
    void set(T&& value) {
        context_[typeid(std::remove_cvref_t<T>).name()] =
            std::make_any<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    template<typename T>
    T get(const std::string& key) const {
        const auto it = context_.find(key);
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
    std::vector<Header> headers_;
    std::unordered_map<std::string, std::any> context_;
};

} // namespace blaze
