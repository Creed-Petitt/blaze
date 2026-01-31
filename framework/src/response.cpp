#include <blaze/response.h>
#include <boost/json/src.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <sstream>
#include <string_view>

namespace blaze {

Response::Response() {
    res_.version(11); // HTTP/1.1
    res_.result(boost::beast::http::status::ok);
}

Response& Response::status(int code) {
    res_.result(code);
    return *this;
}

Response& Response::header(const std::string& key, const std::string& value) {
    res_.set(key, value);
    return *this;
}

Response& Response::add_header(const std::string& key, const std::string& value) {
    res_.insert(key, value);
    return *this;
}

Response& Response::send(const std::string& text) {
    res_.body() = text;
    res_.prepare_payload();
    return *this;
}

Response& Response::json(const boost::json::value& data) {
    res_.set(boost::beast::http::field::content_type, "application/json");
    res_.body() = boost::json::serialize(data);
    res_.prepare_payload();
    return *this;
}

Response& Response::json_raw(std::string_view body) {
    res_.set(boost::beast::http::field::content_type, "application/json");
    res_.body() = std::string(body);
    res_.prepare_payload();
    return *this;
}

std::string Response::build_response() const {
    std::ostringstream oss;
    oss << res_;
    return oss.str();
}

int Response::get_status() const {
    return static_cast<int>(res_.result());
}

Response& Response::redirect(const std::string& url, int code) {
    res_.result(code);
    res_.set(boost::beast::http::field::location, url);
    return *this;
}

Response& Response::set_cookie(const std::string& name, const std::string& value, int max_age_seconds, bool http_only, bool secure) {
    std::string cookie = name + "=" + value;
    if (max_age_seconds > 0) cookie += "; Max-Age=" + std::to_string(max_age_seconds);
    if (http_only) cookie += "; HttpOnly";
    if (secure) cookie += "; Secure";
    cookie += "; Path=/";
    
    res_.insert(boost::beast::http::field::set_cookie, cookie);
    return *this;
}

Response& Response::no_content() {
    res_.result(boost::beast::http::status::no_content);
    res_.body().clear();
    res_.prepare_payload();
    return *this;
}

Response& Response::created(const std::string& location) {
    res_.result(boost::beast::http::status::created);
    if (!location.empty()) {
        res_.set(boost::beast::http::field::location, location);
    }
    return *this;
}

Response& Response::accepted() {
    res_.result(boost::beast::http::status::accepted);
    return *this;
}

Response& Response::bad_request(const std::string& message) {
    res_.result(boost::beast::http::status::bad_request);
    return json({{"error", "Bad Request"}, {"message", message}});
}

Response& Response::unauthorized(const std::string& message) {
    res_.result(boost::beast::http::status::unauthorized);
    return json({{"error", "Unauthorized"}, {"message", message}});
}

Response& Response::forbidden(const std::string& message) {
    res_.result(boost::beast::http::status::forbidden);
    return json({{"error", "Forbidden"}, {"message", message}});
}

Response& Response::not_found(const std::string& message) {
    res_.result(boost::beast::http::status::not_found);
    return json({{"error", "Not Found"}, {"message", message}});
}

} // namespace blaze