#include <blaze/response.h>
#include <boost/json/src.hpp>
#include <sstream>

namespace blaze {

Response::Response() : status_code_(200) {
}

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
    return *this;
}

Response& Response::json(const boost::json::value& data) {
    header("Content-Type", "application/json");
    body_ = boost::json::serialize(data);
    return *this;
}

Response& Response::json_raw(std::string_view body) {
    header("Content-Type", "application/json");
    body_ = std::string(body);
    return *this;
}

std::string Response::build_response() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << " " << get_status_text(status_code_) << "\r\n";
    
    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << "\r\n";
    }

    if (headers_.find("Content-Length") == headers_.end()) {
        oss << "Content-Length: " << body_.length() << "\r\n";
    }

    oss << "\r\n";
    oss << body_;
    return oss.str();
}

int Response::get_status() const {
    return status_code_;
}

Response& Response::redirect(const std::string& url, int code) {
    status(code);
    header("Location", url);
    return *this;
}

Response& Response::no_content() {
    status(204);
    body_.clear();
    return *this;
}

Response& Response::bad_request(const std::string& message) {
    status(400);
    return json({{"error", "Bad Request"}, {"message", message}});
}

Response& Response::unauthorized(const std::string& message) {
    status(401);
    return json({{"error", "Unauthorized"}, {"message", message}});
}

Response& Response::forbidden(const std::string& message) {
    status(403);
    return json({{"error", "Forbidden"}, {"message", message}});
}

Response& Response::not_found(const std::string& message) {
    status(404);
    return json({{"error", "Not Found"}, {"message", message}});
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
        default: return "Unknown Status";
    }
}

} // namespace blaze