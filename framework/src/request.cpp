#include <blaze/request.h>
#include <blaze/util/string.h>
#include <sstream>
#include <algorithm>
#include <charconv>
#include <map>
#include <unordered_map>

namespace blaze {

static std::unordered_map<std::string, std::string> parse_query_params(std::string_view query) {
    std::unordered_map<std::string, std::string> params;
    size_t pos = 0;
    while (pos < query.size()) {
        size_t amp = query.find('&', pos);
        std::string_view pair = query.substr(pos, amp - pos);
        size_t eq = pair.find('=');
        if (eq != std::string_view::npos) {
            std::string key = util::url_decode(pair.substr(0, eq));
            std::string value = util::url_decode(pair.substr(eq + 1));
            params[key] = value;
        }
        if (amp == std::string_view::npos) break;
        pos = amp + 1;
    }
    return params;
}

void Request::set_target(std::string_view target) {
    size_t query_pos = target.find('?');
    if (query_pos != std::string_view::npos) {
        path = std::string(target.substr(0, query_pos));
        query = parse_query_params(target.substr(query_pos + 1));
    } else {
        path = std::string(target);
    }
}

void Request::set_fields(boost::beast::http::fields&& fields) {
    headers = std::move(fields);
}

blaze::Json Request::json() const {
    try {
        return blaze::Json(boost::json::parse(body));
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid JSON in request body: " + std::string(e.what()));
    }
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
    if (it != query.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_val;
        }
    }
    return default_val;
}

std::string_view Request::get_header(std::string_view key) const {
    boost::beast::string_view beast_key(key.data(), key.size());
    auto it = headers.find(beast_key);
    if (it == headers.end()) return "";
    return std::string_view(it->value().data(), it->value().size());
}

bool Request::has_header(std::string_view key) const {
    boost::beast::string_view beast_key(key.data(), key.size());
    return headers.find(beast_key) != headers.end();
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

std::string Request::cookie(const std::string& name) const {
    std::string_view cookie_header = get_header("Cookie");
    if (cookie_header.empty()) return "";

    size_t pos = 0;
    while (pos < cookie_header.size()) {
        size_t end = cookie_header.find(';', pos);
        if (end == std::string_view::npos) end = cookie_header.size();

        std::string_view pair = cookie_header.substr(pos, end - pos);
        
        while (!pair.empty() && std::isspace(pair.front())) pair.remove_prefix(1);
        while (!pair.empty() && std::isspace(pair.back())) pair.remove_suffix(1);

        size_t eq = pair.find('=');
        if (eq != std::string_view::npos) {
            std::string_view key = pair.substr(0, eq);
            std::string_view value = pair.substr(eq + 1);

            while (!key.empty() && std::isspace(key.front())) key.remove_prefix(1);
            while (!key.empty() && std::isspace(key.back())) key.remove_suffix(1);

            if (key == name) {
                while (!value.empty() && std::isspace(value.front())) value.remove_prefix(1);
                while (!value.empty() && std::isspace(value.back())) value.remove_suffix(1);

                if (value.size() >= 2 && value.front() == '\"' && value.back() == '\"') {
                    value.remove_prefix(1);
                    value.remove_suffix(1);
                }
                return std::string(value);
            }
        }
        pos = end + 1;
    }
    return "";
}

std::string Request::url_decode(std::string_view str) {
    return util::url_decode(str);
}

const MultipartFormData& Request::form() const {
    if (cached_form_) return *cached_form_;

    std::string_view content_type = get_header("Content-Type");
    if (content_type.find("multipart/form-data") == std::string_view::npos) {
        cached_form_ = MultipartFormData();
        return *cached_form_;
    }

    auto boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string_view::npos) {
        cached_form_ = MultipartFormData();
        return *cached_form_;
    }

    std::string_view boundary = content_type.substr(boundary_pos + 9);
    if (boundary.size() >= 2 && boundary.front() == '"' && boundary.back() == '"') {
        boundary.remove_prefix(1);
        boundary.remove_suffix(1);
    }

    cached_form_ = multipart::parse(body, boundary);
    return *cached_form_;
}

std::vector<const MultipartPart*> Request::files() const {
    return form().files();
}

} // namespace blaze
