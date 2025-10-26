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

void Response::send_to_client(const int client_fd) const {
    const std::string response = build_response();
    ::send(client_fd, response.c_str(), response.size(), 0);
}

std::string Response::get_status_text(const int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

int Response::get_status() const {
    return status_code_;
}
