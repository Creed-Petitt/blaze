#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <blaze/config.h>
#include <blaze/websocket_registry.h>

#include <optional>
#include <string>
#include <string_view>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace blaze
{
    class RequestDispatcher;

    template <class Stream>
    class HttpSession : public std::enable_shared_from_this<HttpSession<Stream>>
    {
        Stream stream_;
        beast::flat_buffer buffer_;
        std::optional<http::request_parser<http::string_body>> parser_;
        RequestDispatcher& dispatcher_;
        WebSocketRegistry& websockets_;
        const Config& config_;

    public:
        HttpSession(
            RequestDispatcher& dispatcher,
            WebSocketRegistry& websockets,
            const Config& config,
            Stream&& stream);

        void run();
        void do_read();
        void on_read(const beast::error_code& ec, std::size_t bytes_transferred);
        void on_write(bool keep_alive, const beast::error_code& ec, std::size_t bytes_transferred);

        void do_shutdown();

    private:
        std::string get_client_ip();
        void send_error_response(http::status status, std::string_view message);
        bool try_websocket_upgrade();
    };

    using Session = HttpSession<beast::tcp_stream>;
} // namespace blaze
