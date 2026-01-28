#include <blaze/client.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/ssl.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace blaze {

    struct ParsedUrl {
        std::string host;
        std::string port;
        std::string target;
        bool is_ssl;
    };

    ParsedUrl parse_url(const std::string& url) {
        ParsedUrl res;
        std::string s = url;
        
        if (s.substr(0, 8) == "https://") {
            res.is_ssl = true;
            res.port = "443";
            s.erase(0, 8);
        } else if (s.substr(0, 7) == "http://") {
            res.is_ssl = false;
            res.port = "80";
            s.erase(0, 7);
        } else {
            res.is_ssl = false;
            res.port = "80";
        }

        size_t path_pos = s.find('/');
        if (path_pos == std::string::npos) {
            res.host = s;
            res.target = "/";
        } else {
            res.host = s.substr(0, path_pos);
            res.target = s.substr(path_pos);
        }

        size_t port_pos = res.host.find(':');
        if (port_pos != std::string::npos) {
            res.port = res.host.substr(port_pos + 1);
            res.host = res.host.substr(0, port_pos);
        }

        return res;
    }

    std::string resolve_url(const std::string& base, const std::string& relative) {
        if (relative.substr(0, 7) == "http://" || relative.substr(0, 8) == "https://") {
            return relative;
        }
        auto b = parse_url(base);
        std::string proto = b.is_ssl ? "https://" : "http://";
        std::string port_str = (b.port == "80" || b.port == "443") ? "" : ":" + b.port;
        
        if (!relative.empty() && relative[0] == '/') {
            return proto + b.host + port_str + relative;
        }
        
        size_t last_slash = base.find_last_of('/');
        if (last_slash == std::string::npos || last_slash < 8) { // http://...
             return base + "/" + relative;
        }
        return base.substr(0, last_slash + 1) + relative;
    }

    // Static context for performance
    static ssl::context& get_client_ssl_ctx() {
        static ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();
        return ctx;
    }

    boost::asio::awaitable<FetchResponse> fetch_core(
        std::string url, 
        std::string method_str, 
        std::map<std::string, std::string> headers,
        std::string body_str,
        bool set_json_content_type,
        int timeout_seconds
    ) {
        std::string current_url = url;
        std::string current_method = method_str;
        std::string current_body = body_str;
        int redirects = 0;

        while (redirects < 10) {
            auto parsed = parse_url(current_url);
            auto executor = co_await net::this_coro::executor;
            tcp::resolver resolver(executor);

            auto const results = co_await resolver.async_resolve(parsed.host, parsed.port, net::use_awaitable);

            http::request<http::string_body> req;
            req.method(http::string_to_verb(current_method));
            req.target(parsed.target);
            req.version(11);
            req.set(http::field::host, parsed.host);
            req.set(http::field::user_agent, "Blaze/1.0");

            for (auto const& [k, v] : headers) {
                req.set(k, v);
            }

            if (!current_body.empty()) {
                if (set_json_content_type && req.find(http::field::content_type) == req.end()) {
                    req.set(http::field::content_type, "application/json");
                }
                req.body() = current_body;
                req.prepare_payload();
            }

            beast::flat_buffer b;
            http::response<http::string_body> res_msg;

            if (parsed.is_ssl) {
                beast::ssl_stream<beast::tcp_stream> stream(executor, get_client_ssl_ctx());
                beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(timeout_seconds));

                if(! SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str()))
                    throw boost::system::system_error(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());

                co_await beast::get_lowest_layer(stream).async_connect(results, net::use_awaitable);
                co_await stream.async_handshake(ssl::stream_base::client, net::use_awaitable);
                
                co_await http::async_write(stream, req, net::use_awaitable);
                co_await http::async_read(stream, b, res_msg, net::use_awaitable);
                
                beast::error_code ec;
                stream.shutdown(ec);
            } else {
                beast::tcp_stream stream(executor);
                stream.expires_after(std::chrono::seconds(timeout_seconds));

                co_await stream.async_connect(results, net::use_awaitable);
                
                co_await http::async_write(stream, req, net::use_awaitable);
                co_await http::async_read(stream, b, res_msg, net::use_awaitable);
                
                beast::error_code ec;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            }

            // Handle Redirects
            if (res_msg.result_int() >= 300 && res_msg.result_int() < 400) {
                auto it = res_msg.find(http::field::location);
                if (it != res_msg.end()) {
                    current_url = resolve_url(current_url, std::string(it->value()));
                    if (res_msg.result_int() == 303 || res_msg.result_int() == 301 || res_msg.result_int() == 302) {
                        current_method = "GET";
                        current_body = "";
                    }
                    redirects++;
                    continue;
                }
            }

            FetchResponse response;
            response.status = res_msg.result_int();
            
            try {
                response.body = Json(boost::json::parse(res_msg.body()));
            } catch (...) {
                response.body = Json(boost::json::value(res_msg.body()));
            }

            for (auto const& field : res_msg) {
                response.headers.insert({std::string(field.name_string()), std::string(field.value())});
            }

            co_return response;
        }
        throw std::runtime_error("Too many redirects");
    }

    boost::asio::awaitable<FetchResponse> fetch(
        std::string url, 
        std::string method_str, 
        std::map<std::string, std::string> headers,
        Json body,
        int timeout_seconds
    ) {
        std::string body_str;
        bool is_json = false;
        if (body.is_ok()) {
            body_str = boost::json::serialize((boost::json::value)body);
            is_json = true;
        }
        co_return co_await fetch_core(url, method_str, headers, body_str, is_json, timeout_seconds);
    }

    boost::asio::awaitable<FetchResponse> fetch(
        std::string url,
        const MultipartFormData& form,
        int timeout_seconds
    ) {
        auto [body, boundary] = form.encode();
        std::map<std::string, std::string> headers;
        headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;
        
        co_return co_await fetch_core(url, "POST", headers, body, false, timeout_seconds);
    }

} // namespace blaze
