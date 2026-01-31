#include <blaze/util/string.h>
#include <string>
#include <string_view>
#include <cctype>
#include <algorithm>

namespace blaze::util {

namespace {
    inline int hex_to_int(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
}

std::string url_decode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int hi = hex_to_int(str[i + 1]);
            int lo = hex_to_int(str[i + 2]);
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

std::string hex_encode(std::string_view input) {
    static constexpr char hex_chars[] = "0123456789abcdef";
    std::string result;
    result.resize(input.size() * 2);
    char* ptr = result.data();
    for (unsigned char c : input) {
        *ptr++ = hex_chars[c >> 4];
        *ptr++ = hex_chars[c & 0x0f];
    }
    return result;
}

std::string to_snake_case(std::string_view name) {
    // Strip namespace if present (e.g. "blaze::User" -> "User")
    size_t last_colon = name.find_last_of(':');
    std::string_view clean_name = (last_colon == std::string_view::npos) ? name : name.substr(last_colon + 1);

    std::string result;
    result.reserve(clean_name.size() + 5); 
    for (size_t i = 0; i < clean_name.size(); ++i) {
        if (i > 0 && std::isupper(static_cast<unsigned char>(clean_name[i]))) {
            if (!std::isupper(static_cast<unsigned char>(clean_name[i-1])) || 
                (i + 1 < clean_name.size() && std::islower(static_cast<unsigned char>(clean_name[i+1])))) {
                result += '_';
            }
        }
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(clean_name[i])));
    }
    return result;
}

std::string pluralize(std::string_view name) {
    if (name.empty()) return std::string(name);
    
    std::string result(name);
    std::string_view sv(result);

    // Handle s, x, z, ch, sh endings -> +es
    if (sv.ends_with('s') || sv.ends_with('x') || sv.ends_with('z') || 
        sv.ends_with("ch") || sv.ends_with("sh")) {
        result += "es";
    } else if (sv.ends_with('y')) {
        // vowel + y -> +s (e.g. boy -> boys, day -> days)
        // consonant + y -> +ies (e.g. city -> cities, category -> categories)
        if (result.size() > 1) {
            char prev = static_cast<char>(std::tolower(static_cast<unsigned char>(result[result.size() - 2])));
            bool is_vowel = (prev == 'a' || prev == 'e' || prev == 'i' || prev == 'o' || prev == 'u');
            
            if (is_vowel) {
                result += 's';
            } else {
                result.pop_back();
                result += "ies";
            }
        } else {
            // "y" -> "ies"
            result.pop_back();
            result += "ies";
        }
    } else {
        result += 's';
    }
    return result;
}

} // namespace blaze::util
