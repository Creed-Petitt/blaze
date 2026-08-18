#pragma once

#include <string>
#include <string_view>
#include <regex>
#include <blaze/exceptions.h>

namespace blaze::v {

/**
 * @brief Ensures a string is not empty.
 */
inline void required(const std::string_view s, const std::string_view field_name = "Field")  {
    if (s.empty()) throw BadRequest(std::string(field_name) + " is required");
}

/**
 * @brief Validates an email address format.
 */
inline void email(const std::string_view s, const std::string_view field_name = "Email") {
    static const std::regex re(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    if (!std::regex_match(s.begin(), s.end(), re)) {
        throw BadRequest(std::string(field_name) + " is not a valid email address");
    }
}

/**
 * @brief Validates minimum length of a string.
 */
inline void min_len(const std::string_view s, const size_t min, const std::string_view field_name = "Field") {
    if (s.length() < min) {
        throw BadRequest(std::string(field_name) + " must be at least " + std::to_string(min) + " characters");
    }
}

/**
 * @brief Validates maximum length of a string.
 */
inline void max_len(const std::string_view s, const size_t max, const std::string_view field_name = "Field") {
    if (s.length() > max) {
        throw BadRequest(std::string(field_name) + " must not exceed " + std::to_string(max) + " characters");
    }
}

/**
 * @brief Validates a numeric range.
 */
template<typename T>
 void range(T val, T min, T max, const std::string_view field_name = "Value") {
    if (val < min || val > max) {
        throw BadRequest(std::string(field_name) + " must be between " + std::to_string(min) + " and " + std::to_string(max));
    }
}

/**
 * @brief Validates a string against a custom regex.
 */
inline void matches(const std::string_view s, const std::regex& re, const std::string_view field_name = "Field") {
    if (!std::regex_match(s.begin(), s.end(), re)) {
        throw BadRequest(std::string(field_name) + " is invalid");
    }
}

/**
 * @brief Ensures a boolean is true (useful for TOS/Agreements).
 */
inline void is_true(const bool val, const std::string_view field_name = "Field") {
    if (!val) throw BadRequest(std::string(field_name) + " must be accepted");
}

} // namespace blaze::v
