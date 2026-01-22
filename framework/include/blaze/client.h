#ifndef BLAZE_CLIENT_H
#define BLAZE_CLIENT_H

#include <string>
#include <map>
#include <blaze/json.h>
#include <blaze/app.h>
#include <boost/asio/awaitable.hpp>

namespace blaze {

    struct FetchResponse {
        int status;
        Json body;
        std::map<std::string, std::string> headers;

        template<typename T>
        T json() const { return body.as<T>(); }
        std::string text() const { return body.as<std::string>(); }
    };

    /**
     * @brief Performs an asynchronous HTTP/HTTPS request.
     * 
     * @param url The full URL (e.g., "https://api.openai.com/v1/chat")
     * @param method The HTTP method (GET, POST, etc.)
     * @param headers Custom headers
     * @param body Optional JSON body
     * @return boost::asio::awaitable<FetchResponse> 
     */
    boost::asio::awaitable<FetchResponse> fetch(
        std::string url, 
        std::string method = "GET", 
        std::map<std::string, std::string> headers = {},
        Json body = {}
    );

} // namespace blaze

#endif
