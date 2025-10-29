#include "response.h"
#include <sys/socket.h>
#include <sstream>

Response::Response() : status_code_(200) {}

Response& Response::status(const int code) {
    status_code_ = code;
    return *this;
}

Response& Response::header(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

Response &Response::send(const std::string &text) {
    body_ = text;
    headers_["Content-Length"] = std::to_string(body_.size());
    return *this;
}

Response& Response::json(const nlohmann::json& data) {
    body_ = data.dump();
    headers_["Content-Type"] = "application/json";
    headers_["Content-Length"] = std::to_string(body_.size());
    return *this;
}

std::string Response::build_response() const {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status_code_ << " " << get_status_text(status_code_) << "\r\n";

    for (const auto& [key, value] : headers_) {
        ss << key << ": " << value << "\r\n";
    }

    ss << "\r\n";
    ss << body_;
    return ss.str();
}

void Response::write(const int client_fd) const {
    const std::string response = build_response();
    ::send(client_fd, response.c_str(), response.size(), 0);
}

std::string Response::get_status_text(const int code) {
    switch (code) {
        // 2xx Success
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";

        // 3xx Redirection
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        // 4xx Client Errors
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 413: return "Payload Too Large";
        case 415: return "Unsupported Media Type";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";

        // 5xx Server Errors
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";

        default: return "Unknown";
    }
}

int Response::get_status() const {
    return status_code_;
}

// Helper methods for common response patterns
Response& Response::redirect(const std::string& url, int code) {
    return status(code).header("Location", url).send("");
}

Response& Response::no_content() {
    return status(204).send("");
}

Response& Response::bad_request(const std::string& message) {
    return status(400).json({{"error", message}});
}

Response& Response::unauthorized(const std::string& message) {
    return status(401).json({{"error", message}});
}

Response& Response::forbidden(const std::string& message) {
    return status(403).json({{"error", message}});
}

Response& Response::not_found(const std::string& message) {
    return status(404).json({{"error", message}});
}
