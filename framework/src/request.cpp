#include <blaze/request.h>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace blaze {

void Request::set_target(std::string_view target) {
    const size_t query_pos = target.find('?');

    if (query_pos != std::string_view::npos) {
        path = target.substr(0, query_pos);
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
        path = target;
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

} // namespace blaze