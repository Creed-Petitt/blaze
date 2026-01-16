#include <blaze/response.h>
#include <sstream>

namespace blaze {

Response::Response() : status_code_(200) {}

Response& Response::status(int code) {
    status_code_ = code;
    return *this;
}

Response& Response::header(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

Response& Response::send(const std::string& text) {
    body_ = text;
    if (headers_.find("Content-Type") == headers_.end()) {
        headers_["Content-Type"] = "text/plain";
    }
    return *this;
}

Response& Response::json(const boost::json::value& data) {
    body_ = boost::json::serialize(data);
    headers_["Content-Type"] = "application/json";
    return *this;
}

Response& Response::json_raw(std::string_view body) {
    body_ = std::string(body);
    headers_["Content-Type"] = "application/json";
    return *this;
}

std::string Response::build_response() const {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status_code_ << " " << get_status_text(status_code_) << "\r\n";
    
    for (const auto& [key, value] : headers_) {
        ss << key << ": " << value << "\r\n";
    }
    
    ss << "Content-Length: " << body_.size() << "\r\n";
    ss << "\r\n";
    ss << body_;
    
    return ss.str();
}

int Response::get_status() const {
    return status_code_;
}

// ... helper methods ...

Response& Response::redirect(const std::string& url, int code) {
    status_code_ = code;
    headers_["Location"] = url;
    return *this;
}

Response& Response::no_content() {
    status_code_ = 204;
    return *this;
}

Response& Response::bad_request(const std::string& message) {
    status_code_ = 400;
    json({{"error", "Bad Request"}, {"message", message}});
    return *this;
}

Response& Response::unauthorized(const std::string& message) {
    status_code_ = 401;
    json({{"error", "Unauthorized"}, {"message", message}});
    return *this;
}

Response& Response::forbidden(const std::string& message) {
    status_code_ = 403;
    json({{"error", "Forbidden"}, {"message", message}});
    return *this;
}

Response& Response::not_found(const std::string& message) {
    status_code_ = 404;
    json({{"error", "Not Found"}, {"message", message}});
    return *this;
}

std::string Response::get_status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

} // namespace blaze