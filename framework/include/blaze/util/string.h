#ifndef BLAZE_UTIL_STRING_H
#define BLAZE_UTIL_STRING_H

#include <string>
#include <string_view>

namespace blaze::util {

/**
 * @brief Decodes a URL-encoded string (e.g., %20 to space).
 * Handles both '+' and '%xx' encodings.
 */
std::string url_decode(std::string_view str);

} // namespace blaze::util

#endif // BLAZE_UTIL_STRING_H
