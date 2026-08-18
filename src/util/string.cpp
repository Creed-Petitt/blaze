#include <blaze/util/string.h>
#include <string>
#include <string_view>
#include <cctype>
#include <algorithm>

namespace blaze::util {

namespace {
    int hex_to_int(const char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
}

std::string url_decode(const std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            const int hi = hex_to_int(str[i + 1]);
            const int lo = hex_to_int(str[i + 2]);
            if (hi != -1 && lo != -1) {
                result += static_cast<char>((hi << 4) | lo);
                i += 2;
            } else {
                result += '%';
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string hex_encode(const std::string_view input) {
    static constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.resize(input.size() * 2);
    char* ptr = result.data();
    for (const unsigned char c : input) {
        *ptr++ = hex_chars[c >> 4];
        *ptr++ = hex_chars[c & 0x0f];
    }
    return result;
}

} // namespace blaze::util
