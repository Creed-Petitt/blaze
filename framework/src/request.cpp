#include <blaze/request.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace blaze {

namespace {
    std::string to_lower(std::string s) {
        for (auto& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }
}

void Request::set_target(std::string_view target) {
    const size_t query_pos = target.find('?');

    if (query_pos != std::string_view::npos) {
        path = std::string(target.substr(0, query_pos));
        std::string_view query_str = target.substr(query_pos + 1);

        // Parse Query Params
        size_t pos = 0;
        while (pos < query_str.size()) {
            size_t amp_pos = query_str.find('&', pos);
            if (amp_pos == std::string_view::npos) amp_pos = query_str.size();

            std::string_view pair = query_str.substr(pos, amp_pos - pos);
            const size_t eq_pos = pair.find('=');

            if (eq_pos != std::string_view::npos) {
                auto key = std::string(pair.substr(0, eq_pos));
                auto value = std::string(pair.substr(eq_pos + 1));
                query[std::move(key)] = std::move(value);
            }
            pos = amp_pos + 1;
        }
    } else {
        path = std::string(target);
    }
}

void Request::add_header(const std::string_view key, const std::string_view value) {
    std::string k{key};
    for(auto& c : k)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    headers[std::move(k)] = std::string(value);
}

nlohmann::json Request::json() const {
    try {
        return nlohmann::json::parse(body);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Invalid JSON in request body: " + std::string(e.what()));
    }
}

// Helper methods for safe parameter and header access
std::string Request::get_query(const std::string& key, const std::string& default_val) const {
    const auto it = query.find(key);
    if (it != query.end()) {
        return it->second;
    }
    return default_val;
}

int Request::get_query_int(const std::string& key, const int default_val) const {
    const auto it = query.find(key);
    if (it != query.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_val;
        }
    }
    return default_val;
}

std::string Request::get_header(const std::string& key, const std::string& default_val) const {

    const std::string normalized = to_lower(key);
    const auto it = headers.find(normalized);
    if (it != headers.end()) {
        return it->second;
    }
    return default_val;
}

bool Request::has_header(const std::string& key) const {
    return headers.contains(to_lower(key));
}

std::optional<int> Request::get_param_int(const std::string& key) const {
    const auto it = params.find(key);
    if (it != params.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace blaze