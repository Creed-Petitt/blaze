#include <blaze/request.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace {
    std::string to_lower(std::string s) {
        for (auto& c : s) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return s;
    }
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