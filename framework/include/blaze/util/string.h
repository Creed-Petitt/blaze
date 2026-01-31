#ifndef BLAZE_UTIL_STRING_H
#define BLAZE_UTIL_STRING_H

#include <string>
#include <string_view>
#include <type_traits>
#include <charconv>
#include <blaze/exceptions.h>

namespace blaze::util {

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
 * @brief Converts CamelCase or PascalCase to snake_case.
 */
std::string to_snake_case(std::string_view name);

/**
 * @brief Simple English pluralization helper (e.g., user -> users, category -> categories).
 */
std::string pluralize(std::string_view name);

/**
 * @brief Converts various types to string for database parameters.
 */
template<typename T>
std::string to_string_param(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) return val;
    else if constexpr (std::is_constructible_v<std::string, T>) return std::string(val);
    else if constexpr (std::is_same_v<T, bool>) return val ? "true" : "false";
    else return std::to_string(val);
}

/**
 * @brief Efficiently converts a string_view to a numeric or boolean type.
 * Uses std::from_chars for high-performance parsing without allocations.
 * @throws HttpError(400) if parsing fails.
 */
template<typename T>
T convert_string(std::string_view s) {
    using PureT = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<PureT, std::string>) {
        return std::string(s);
    } else if constexpr (std::is_same_v<PureT, bool>) {
        if (s == "true" || s == "1" || s == "yes" || s == "t") return true;
        if (s == "false" || s == "0" || s == "no" || s == "f") return false;
        throw HttpError(400, "Invalid boolean format: " + std::string(s));
    } else if constexpr (std::is_integral_v<PureT>) {
        PureT val;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec != std::errc()) throw HttpError(400, "Invalid integer format: " + std::string(s));
        return val;
    } else if constexpr (std::is_floating_point_v<PureT>) {
        // MacOS fallback
        try {
            return static_cast<PureT>(std::stod(std::string(s)));
        } catch (...) {
            throw HttpError(400, "Invalid floating point format: " + std::string(s));
        }
    } else {
        return PureT{};
    }
}

} // namespace blaze::util

namespace blaze {
    using util::to_string_param;
    using util::convert_string;
}

#endif // BLAZE_UTIL_STRING_H
