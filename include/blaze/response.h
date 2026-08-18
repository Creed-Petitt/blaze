#pragma once

#include <blaze/header.h>
#include <blaze/json.h>

#include <optional>
#include <type_traits>
#include <string>
#include <string_view>
#include <initializer_list>
#include <utility>
#include <vector>

namespace blaze {

class Response {

public:
    // Status and headers
    Response& status(int code);
    Response& header(const std::string& key, const std::string& value);
    Response& set_header(const std::string& key, const std::string& value);
    Response& add_header(const std::string& key, const std::string& value);
    Response& headers(std::initializer_list<std::pair<std::string, std::string>> headers);

    // Payload / Body
    Response& send(const std::string& text);
    Response& file(const std::string& path);
    Response& json(const Json& data);
    template<typename T>
    Response& json(const T& data) {
        return json(Json(data));
    }
    Response& json_raw(std::string_view body);


    // Getters / Inspection
    int get_status() const;
    const std::string& get_body() const;
    const std::vector<Header>& get_headers() const;
    bool is_file() const;
    const std::string& get_file_path() const;
    std::string build_response() const;


    // Helper methods for common response patterns
    Response& redirect(const std::string& url, int code = 302);
    Response& no_content();
    Response& created(const std::string& location = "");
    Response& accepted();
    Response& bad_request(const std::string& message);
    Response& unauthorized(const std::string& message = "Unauthorized");
    Response& forbidden(const std::string& message = "Forbidden");
    Response& not_found(const std::string& message = "Not Found");


private:
    int status_code_{200};
    std::vector<Header> headers_;
    std::string body_;
    std::optional<std::string> file_path_;
};

} // namespace blaze
