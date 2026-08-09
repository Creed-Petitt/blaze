#pragma once

#include <string>
#include <optional>
#include <cstdlib>
#include <stdexcept>
#include <charconv>
#include <iostream>

namespace blaze {

    bool load_env(const std::string& path = ".env");

    template <typename T = std::string>
    T env(const std::string& key, std::optional<T> default_value = std::nullopt) {
        const char* val = std::getenv(key.c_str());

        if (val == nullptr) {
            if (default_value.has_value()) {
                return default_value.value();
            }
            throw std::runtime_error("Missing environment variable: " + key);
        }

        std::string s_val = val;

        if constexpr (std::is_same_v<T, std::string>) {
            return s_val;
        } 
        else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(s_val);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::stod(s_val);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return (s_val == "true" || s_val == "1" || s_val == "yes");
        }
        else {
            static_assert(sizeof(T) == 0, "Unsupported type for blaze::env");
        }
    }
}
