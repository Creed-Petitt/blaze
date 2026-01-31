#include <blaze/util/string.h>
#include <sstream>
#include <iomanip>

namespace blaze::util {

std::string url_decode(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream is(std::string(str.substr(i + 1, 2)));
            if (is >> std::hex >> value) {
                result += static_cast<char>(value);
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

} // namespace blaze::util
