#pragma once

#include <blaze/header.h>
#include <blaze/json.h>
#include <blaze/util/string.h>

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

    template<typename T>
    std::optional<T> query_as(std::string_view key) const {
        for (const auto& [name, value] : query) {
            if (name == key) {
                auto parsed = util::parse_value<T>(value);
                if (parsed) return std::move(*parsed);
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    template<typename T>
    std::optional<T> param_as(std::string_view key) const {
        for (const auto& [name, value] : params) {
            if (name == key) {
                auto parsed = util::parse_value<T>(value);
                if (parsed) return std::move(*parsed);
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    // Body parsing
    std::expected<Json, util::ParseError> try_json() const;
    Json json() const;

    template<typename T>
    T json() const {
        return this->json().template as<T>();
    }

    template<typename T>
    std::expected<T, util::ParseError> try_json_as() const {
        auto parsed = try_json();
        if (!parsed) {
            return std::unexpected(parsed.error());
        }

        try {
            return parsed->template as<T>();
        } catch (const std::exception& e) {
            return std::unexpected(util::ParseError{
                util::ParseErrorCode::Invalid,
                "Invalid JSON body: " + std::string(e.what())
            });
        }
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
