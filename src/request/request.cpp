#include <blaze/request.h>
#include <blaze/exceptions.h>
#include <blaze/util/string.h>

#include <boost/json.hpp>

#include <algorithm>
#include <cctype>

namespace blaze {

namespace {

bool header_name_equal(const std::string_view lhs, const std::string_view rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
               [](const unsigned char a, const unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

std::unordered_map<std::string, std::string> parse_query_params(std::string_view query) {
    std::unordered_map<std::string, std::string> params;
    size_t pos = 0;
    while (pos < query.size()) {
        const size_t amp = query.find('&', pos);
        const std::string_view pair = query.substr(pos, amp - pos);
        const size_t eq = pair.find('=');
        if (eq != std::string_view::npos) {
            params[util::url_decode(pair.substr(0, eq))] = util::url_decode(pair.substr(eq + 1));
        }
        if (amp == std::string_view::npos) break;
        pos = amp + 1;
    }
    return params;
}

} // namespace

void Request::set_target(std::string_view target) {
    query.clear();

    const size_t query_pos = target.find('?');
    if (query_pos == std::string_view::npos) {
        path = std::string(target);
        return;
    }

    path = std::string(target.substr(0, query_pos));
    query = parse_query_params(target.substr(query_pos + 1));
}

void Request::set_header(const std::string_view key, const std::string_view value) {
    for (auto& [name, existing_value] : headers_) {
        if (header_name_equal(name, key)) {
            name = std::string(key);
            existing_value = std::string(value);
            return;
        }
    }

    add_header(key, value);
}

void Request::add_header(const std::string_view key, const std::string_view value) {
    headers_.push_back({std::string(key), std::string(value)});
}

std::string_view Request::get_header(std::string_view key) const {
    for (const auto& [name, value] : headers_) {
        if (header_name_equal(name, key)) {
            return value;
        }
    }
    return "";
}

bool Request::has_header(std::string_view key) const {
    return std::ranges::any_of(headers_, [key](const Header& header) {
        return header_name_equal(header.name, key);
    });
}

std::string Request::url_decode(const std::string_view str) {
    return util::url_decode(str);
}

std::string Request::get_query(const std::string& key, const std::string& default_val) const {
    const auto it = query.find(key);
    if (it != query.end()) {
        return it->second;
    }
    return default_val;
}

int Request::get_query_int(const std::string& key, const int default_val) const {
    const auto it = query.find(key);
    if (it == query.end()) {
        return default_val;
    }

    try {
        return std::stoi(it->second);
    } catch (const std::exception&) {
        return default_val;
    }
}

std::optional<int> Request::get_param_int(const std::string& key) const {
    const auto it = params.find(key);
    if (it == params.end()) {
        return std::nullopt;
    }

    try {
        return std::stoi(it->second);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

Json Request::json() const {
    try {
        return Json(boost::json::parse(body));
    } catch (const std::exception& e) {
        throw BadRequest("Invalid JSON in request body: " + std::string(e.what()));
    }
}

} // namespace blaze
