#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <string>
#include <string_view>
#include <boost/beast/http/fields.hpp>
#include <boost/json.hpp>
#include <blaze/json.h>

namespace blaze {

class Response {
private:
    int status_code_ = 0;
    boost::beast::http::fields headers_;
    std::string body_;
    static std::string get_status_text(int code);

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
    
    // Boost.JSON overload
    Response& json(const boost::json::value& data);

    //Generic JSON Serializer
    template<typename T>
    Response& json(const T& data) {
        if constexpr (std::is_convertible_v<T, boost::json::value>) {
            return json(static_cast<boost::json::value>(data));
        } else {
            return json(boost::json::value_from(data));
        }
    }
    
    // Raw JSON string
    Response& json_raw(std::string_view body);

    std::string build_response() const;
    int get_status() const;

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
