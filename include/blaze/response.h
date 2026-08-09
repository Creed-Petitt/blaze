#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <string>
#include <string_view>
#include <cstdint>
#include <optional>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/json.hpp>
#include <blaze/json.h>

namespace blaze {

class Response {
private:
    boost::beast::http::response<boost::beast::http::string_body> res_;
    std::optional<std::string> file_path_;

public:
    Response();

    Response& status(int code);
    Response& header(const std::string& key, const std::string& value);
    Response& add_header(const std::string& key, const std::string& value);

    Response& headers(std::initializer_list<std::pair<std::string, std::string>> headers) {
        for (const auto& [key, value] : headers) {
            header(key, value);
        }
        return *this;
    }

    Response& send(const std::string& text);
    Response& file(const std::string& path);
    
    // Boost.JSON overload
    Response& json(const boost::json::value& data);

    // Generic JSON Serializer
    template<typename T>
    Response& json(const T& data) {
        if constexpr (std::is_same_v<T, Json>) {
            return json(data.value());
        } else if constexpr (std::is_convertible_v<T, boost::json::value>) {
            return json(static_cast<boost::json::value>(data));
        } else {
            return json(boost::json::value_from(data));
        }
    }
    
    // Raw JSON string
    Response& json_raw(std::string_view body);

    std::string build_response() const;
    int get_status() const;

    bool is_file() const { return file_path_.has_value(); }
    const std::string& get_file_path() const { return *file_path_; }
    const boost::beast::http::response<boost::beast::http::string_body>& get_beast_response() const { return res_; }
    boost::beast::http::response<boost::beast::http::string_body>& get_beast_response() { return res_; }

    // Helper methods for common response patterns
    Response& redirect(const std::string& url, int code = 302);
    Response& set_cookie(const std::string& name, const std::string& value, 
                        int max_age_seconds = 0, bool http_only = true, bool secure = false);
    Response& no_content();
    Response& created(const std::string& location = "");
    Response& accepted();
    Response& bad_request(const std::string& message);
    Response& unauthorized(const std::string& message = "Unauthorized");
    Response& forbidden(const std::string& message = "Forbidden");
    Response& not_found(const std::string& message = "Not Found");

};

} // namespace blaze

#endif
