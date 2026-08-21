#include <blaze/response.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace blaze {

namespace {

bool header_name_equal(const std::string_view lhs, const std::string_view rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(),
               [](const unsigned char a, const unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

std::string_view http_to_msg(const int status) {
    switch (status) {
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 413: return "Payload Too Large";
    case 429: return "Too Many Requests";
    case 500: return "Internal Server Error";
    default: return "Unknown";
    }
}

} // namespace

Response& Response::status(const int code) {
    status_code_ = code;
    return *this;
}

Response& Response::header(const std::string& key, const std::string& value) {
    return set_header(key, value);
}

Response& Response::set_header(const std::string& key, const std::string& value) {
    for (auto& [name, existing_value] : headers_) {
        if (header_name_equal(name, key)) {
            name = key;
            existing_value = value;
            return *this;
        }
    }

    return add_header(key, value);
}

Response& Response::add_header(const std::string& key, const std::string& value) {
    headers_.push_back({key, value});
    return *this;
}

Response& Response::headers(std::initializer_list<std::pair<std::string, std::string>> headers) {
    for (const auto& [key, value] : headers) {
        set_header(key, value);
    }
    return *this;
}

Response& Response::send(const std::string& text) {
    body_ = text;
    file_path_.reset();
    return *this;
}

Response& Response::file(const std::string& path) {
    file_path_ = path;
    body_.clear();
    return *this;
}

Response& Response::json(const Json& data) {
    set_header("Content-Type", "application/json");
    body_ = data.dump();
    file_path_.reset();
    return *this;
}

Response& Response::json_raw(const std::string_view body) {
    set_header("Content-Type", "application/json");
    body_ = std::string(body);
    file_path_.reset();
    return *this;
}

std::string Response::build_response() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << ' ' << http_to_msg(status_code_) << "\r\n";
    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << "\r\n";
    }
    if (!is_file()) {
        oss << "Content-Length: " << body_.size() << "\r\n";
    }
    oss << "\r\n";
    if (!is_file()) {
        oss << body_;
    }
    return oss.str();
}

int Response::get_status() const {
    return status_code_;
}

const std::string& Response::get_body() const {
    return body_;
}

const std::vector<Header>& Response::get_headers() const {
    return headers_;
}

bool Response::is_file() const {
    return file_path_.has_value();
}

const std::string& Response::get_file_path() const {
    if (!file_path_) {
        throw std::logic_error("Response does not contain a file path");
    }
    return *file_path_;
}

Response& Response::redirect(const std::string& url, const int code) {
    status_code_ = code;
    set_header("Location", url);
    return *this;
}

Response& Response::no_content() {
    status_code_ = 204;
    body_.clear();
    file_path_.reset();
    return *this;
}

Response& Response::created(const std::string& location) {
    status_code_ = 201;
    if (!location.empty()) {
        set_header("Location", location);
    }
    return *this;
}

Response& Response::accepted() {
    status_code_ = 202;
    return *this;
}

Response& Response::bad_request(const std::string& message) {
    status_code_ = 400;
    return json({{"error", "Bad Request"}, {"message", message}});
}

Response& Response::unauthorized(const std::string& message) {
    status_code_ = 401;
    return json({{"error", "Unauthorized"}, {"message", message}});
}

Response& Response::forbidden(const std::string& message) {
    status_code_ = 403;
    return json({{"error", "Forbidden"}, {"message", message}});
}

Response& Response::not_found(const std::string& message) {
    status_code_ = 404;
    return json({{"error", "Not Found"}, {"message", message}});
}

} // namespace blaze
