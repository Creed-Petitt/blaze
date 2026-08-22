#pragma once

#include <blaze/exceptions.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <charconv>
#include <expected>
#include <system_error>
#include <utility>

namespace blaze::util {

enum class ParseErrorCode {
    Invalid,
    OutOfRange,
    UnsupportedType
};

struct ParseError {
    ParseErrorCode code;
    std::string message;
};

/**
 * @brief Decodes a URL-encoded string (e.g., %20 to space).
 * Handles both '+' and '%xx' encodings.
 */
std::string url_decode(std::string_view str);

/**
 * @brief Converts a binary string or buffer to a hex-encoded string.
 */
std::string hex_encode(std::string_view input);

/**
 * @brief Converts various types to string for route/query parameter handling.
 */
template<typename T>
std::string to_string_param(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) return val;
    else if constexpr (std::is_constructible_v<std::string, T>) return std::string(val);
    else if constexpr (std::is_same_v<T, bool>) return val ? "true" : "false";
    else return std::to_string(val);
}

template<typename T>
std::expected<T, ParseError> parse_value(std::string_view s) {
    using PureT = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<PureT, std::string>) {
        return std::string(s);
    } else if constexpr (std::is_same_v<PureT, bool>) {
        if (s == "true" || s == "1" || s == "yes" || s == "t") return true;
        if (s == "false" || s == "0" || s == "no" || s == "f") return false;
        return std::unexpected(ParseError{
            ParseErrorCode::Invalid,
            "Invalid boolean format: " + std::string(s)
        });
    } else if constexpr (std::is_integral_v<PureT>) {
        PureT val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected(ParseError{
                ParseErrorCode::OutOfRange,
                "Integer out of range: " + std::string(s)
            });
        }
        if (ec != std::errc() || ptr != s.data() + s.size()) {
            return std::unexpected(ParseError{
                ParseErrorCode::Invalid,
                "Invalid integer format: " + std::string(s)
            });
        }
        return val;
    } else if constexpr (std::is_floating_point_v<PureT>) {
        PureT val{};
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected(ParseError{
                ParseErrorCode::OutOfRange,
                "Floating point value out of range: " + std::string(s)
            });
        }
        if (ec != std::errc() || ptr != s.data() + s.size()) {
            return std::unexpected(ParseError{
                ParseErrorCode::Invalid,
                "Invalid floating point format: " + std::string(s)
            });
        }
        return val;
    } else {
        return std::unexpected(ParseError{
            ParseErrorCode::UnsupportedType,
            "Unsupported conversion type"
        });
    }
}

/**
 * @brief Compatibility wrapper for APIs that still surface parse failures as HTTP 400 exceptions.
 */
template<typename T>
T convert_string(std::string_view s) {
    auto parsed = parse_value<T>(s);
    if (!parsed) {
        throw HttpError(400, parsed.error().message);
    }
    return std::move(*parsed);
}

} // namespace blaze::util

namespace blaze {
    using util::to_string_param;
    using util::parse_value;
    using util::convert_string;
}
