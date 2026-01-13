#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <json.hpp>

using json = nlohmann::json;

class Response {
private:
    int status_code_ = 0;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
    static std::string get_status_text(int code);

public:
    Response();

    Response& status(int code);
    Response& header(const std::string& key, const std::string& value);
    Response& send(const std::string& text);
    Response& json(const nlohmann::json& data);
    Response& json_raw(std::string_view body);

    std::string build_response() const;
    int get_status() const;

    // Helper methods for common response patterns
    Response& redirect(const std::string& url, int code = 302);
    Response& no_content();
    Response& bad_request(const std::string& message);
    Response& unauthorized(const std::string& message = "Unauthorized");
    Response& forbidden(const std::string& message = "Forbidden");
    Response& not_found(const std::string& message = "Not Found");

};

#endif