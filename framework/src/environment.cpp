#include "blaze/environment.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace blaze {

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (std::string::npos == first) {
            return str;
        }
        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, (last - first + 1));
    }

    bool load_env(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false; 
        }

        std::string line;
        while (std::getline(file, line)) {
            std::string clean_line = trim(line);

            if (clean_line.empty() || clean_line[0] == '#') {
                continue;
            }

            size_t delimiter_pos = clean_line.find('=');
            if (delimiter_pos == std::string::npos) {
                continue; // Invalid line
            }

            std::string key = trim(clean_line.substr(0, delimiter_pos));
            std::string value = trim(clean_line.substr(delimiter_pos + 1));

            if (value.size() >= 2 && 
               ((value.front() == '"' && value.back() == '"') || 
                (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }

            // Set environment variable (overwrite if exists)
            setenv(key.c_str(), value.c_str(), 1);
        }

        return true;
    }
}
