#ifndef BLAZE_CLIENT_H
#define BLAZE_CLIENT_H

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <blaze/json.h>
#include <blaze/app.h>
#include <blaze/multipart.h>
#include <boost/asio/awaitable.hpp>

namespace blaze {

    struct CaseInsensitiveCompare {
        bool operator()(const std::string& a, const std::string& b) const {
            return std::lexicographical_compare(
                a.begin(), a.end(), b.begin(), b.end(),
                [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); }
            );
        }
    };

    struct FetchResponse {
        int status;
        Json body;
        
        std::multimap<std::string, std::string, CaseInsensitiveCompare> headers;

        template<typename T>
        T json() const { return body.as<T>(); }
        std::string text() const { return body.as<std::string>(); }

        // Get first value
        std::string get_header(const std::string& key) const {
            auto it = headers.find(key);
            if (it != headers.end()) return it->second;
            return "";
        }

        // Get all values for a key
        std::vector<std::string> get_headers(const std::string& key) const {
            std::vector<std::string> values;
            auto range = headers.equal_range(key);
            for (auto it = range.first; it != range.second; ++it) {
                values.push_back(it->second);
            }
            return values;
        }
    };

    /**
     * @brief Performs an asynchronous HTTP/HTTPS request.
     * 
     * @param url The full URL
     * @param method HTTP method (GET, POST, etc.)
     * @param headers Custom headers
     * @param body Optional JSON body
     * @param timeout_seconds Request timeout (default: 30s)
     * @return boost::asio::awaitable<FetchResponse> 
     */
    boost::asio::awaitable<FetchResponse> fetch(
        std::string url, 
        std::string method = "GET", 
        std::map<std::string, std::string> headers = {},
        Json body = {},
        int timeout_seconds = 30
    );

    inline boost::asio::awaitable<FetchResponse> fetch(
        std::string url, 
        std::string method, 
        Json body,
        int timeout_seconds = 30
    ) {
        co_return co_await fetch(std::move(url), std::move(method), {}, std::move(body), timeout_seconds);
    }

    /**
     * @brief Performs a multipart/form-data upload.
     */
    boost::asio::awaitable<FetchResponse> fetch(
        std::string url,
        const MultipartFormData& form,
        int timeout_seconds = 30
    );

} // namespace blaze

#endif
