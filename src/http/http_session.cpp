#include "http_session.h"
#include "../request/request_dispatcher.h"
#include "../websocket/websocket_session.h"

#include <blaze/exceptions.h>
#include <blaze/request.h>
#include <blaze/response.h>

#include <boost/beast/http/file_body.hpp>

#include <iostream>
#include <tuple>
#include <utility>

namespace blaze {

template<class Stream>
struct StreamTraits {
    static void shutdown(Stream& stream) {
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

    static tcp::socket& get_socket(Stream& stream) {
        return stream.socket();
    }
};

namespace {

Request make_request(http::request<http::string_body>&& req) {
    Request blaze_req;
    blaze_req.method = std::string(req.method_string());
    blaze_req.set_target(std::string_view(req.target().data(), req.target().size()));
    blaze_req.body = std::move(req.body());

    for (const auto& field : req) {
        blaze_req.add_header(field.name_string(), field.value());
    }

    return blaze_req;
}

template <typename Stream>
boost::asio::awaitable<void> write_error(
    Stream& stream,
    const http::status status,
    const std::string_view message,
    const unsigned version = 11
) {
    http::response<http::string_body> res{status, version};
    res.set(http::field::content_type, "text/plain");
    res.body() = std::string(message);
    res.prepare_payload();
    co_await http::async_write(stream, res, net::use_awaitable);
}

template <typename Stream>
boost::asio::awaitable<void> write_response(
    Stream& stream,
    const Response& blaze_res,
    const bool keep_alive,
    const unsigned version = 11
) {
    if (blaze_res.is_file()) {
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(blaze_res.get_file_path().c_str(), beast::file_mode::scan, ec);

        if (ec) {
            co_await write_error(stream, http::status::not_found, "File not found", version);
            co_return;
        }

        const auto size = body.size();
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(static_cast<http::status>(blaze_res.get_status()), version)
        };

        for (const auto& [name, value] : blaze_res.get_headers()) {
            res.set(name, value);
        }

        res.content_length(size);
        res.keep_alive(keep_alive);
        res.prepare_payload();
        co_await http::async_write(stream, res, net::use_awaitable);
        co_return;
    }

    http::response<http::string_body> res{
        static_cast<http::status>(blaze_res.get_status()),
        version
    };

    for (const auto& [name, value] : blaze_res.get_headers()) {
        res.set(name, value);
    }

    res.body() = blaze_res.get_body();
    res.keep_alive(keep_alive);
    res.prepare_payload();
    co_await http::async_write(stream, res, net::use_awaitable);
}

template <typename Stream, typename SessionPtr>
boost::asio::awaitable<void> handle_session(
    Stream& stream,
    SessionPtr self,
    RequestDispatcher& dispatcher,
    Request req,
    const std::string& client_ip,
    bool keep_alive
) {
    Response blaze_res;
    bool error_occurred = false;
    std::optional<std::pair<http::status, std::string>> pending_error;

    try {
        blaze_res = co_await dispatcher.handle(req, client_ip, keep_alive);
    } catch (const HttpError& e) {
        pending_error = std::make_pair(static_cast<http::status>(e.status()), std::string(e.what()));
        keep_alive = false;
    } catch (const boost::system::system_error& e) {
        if (e.code() != net::error::operation_aborted) {
            std::cerr << "Async Handler Error: " << e.what() << "\n";
        }
        error_occurred = true;
    } catch (const std::exception& e) {
        std::cerr << "Async Handler Error: " << e.what() << "\n";
        pending_error = std::make_pair(http::status::internal_server_error, "Internal Server Error");
        keep_alive = false;
    }

    if (pending_error) {
        try {
            co_await write_error(stream, pending_error->first, pending_error->second, 11);
        } catch (...) {
            error_occurred = true;
        }
    } else {
        try {
            co_await write_response(stream, blaze_res, keep_alive, 11);
        } catch (...) {
            error_occurred = true;
        }
    }

    if (error_occurred || !keep_alive) {
        StreamTraits<Stream>::shutdown(stream);
    } else {
        self->do_read();
    }
}

} // namespace

template<class Stream>
HttpSession<Stream>::HttpSession(
    RequestDispatcher& dispatcher,
    WebSocketRegistry& websockets,
    const Config& config,
    Stream&& stream)
    : stream_(std::move(stream)),
      dispatcher_(dispatcher),
      websockets_(websockets),
      config_(config) {}

template<class Stream>
void HttpSession<Stream>::run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&HttpSession::do_read, this->shared_from_this()));
}

template<class Stream>
void HttpSession<Stream>::do_read() {
    parser_.emplace();
    parser_->body_limit(config_.max_body_size);

    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(config_.timeout_seconds)
    );

    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(&HttpSession::on_read, this->shared_from_this()));
}

template<class Stream>
void HttpSession<Stream>::on_read(const beast::error_code& ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        do_shutdown();
        return;
    }

    if (ec) {
        if (ec == http::error::body_limit) {
            send_error_response(http::status::payload_too_large, "Payload Too Large");
            return;
        }

        if (ec != net::error::connection_reset && ec != net::error::eof && ec != beast::error::timeout) {
            std::cerr << "Request Parse Error: " << ec.message() << "\n";
            send_error_response(http::status::bad_request, "Bad Request");
            return;
        }
        return;
    }

    if (try_websocket_upgrade()) {
        return;
    }

    auto beast_req = parser_->release();
    const bool keep_alive = beast_req.keep_alive();
    std::string client_ip = get_client_ip();

    boost::asio::co_spawn(
        stream_.get_executor(),
        handle_session(
            stream_,
            this->shared_from_this(),
            dispatcher_,
            make_request(std::move(beast_req)),
            client_ip,
            keep_alive
        ),
        boost::asio::detached
    );
}

template<class Stream>
void HttpSession<Stream>::on_write(
    const bool keep_alive,
    const beast::error_code& ec,
    std::size_t bytes_transferred) {

        boost::ignore_unused(bytes_transferred);
        if (ec) return;
        if (!keep_alive) {
            do_shutdown();
            return;
        }
    do_read();
}

template<class Stream>
void HttpSession<Stream>::do_shutdown() {
    beast::get_lowest_layer(stream_).
        expires_after(std::chrono::seconds(30));
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

template<class Stream>
std::string HttpSession<Stream>::get_client_ip() {
    try {
        return StreamTraits<Stream>::get_socket(stream_).remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

template<class Stream>
void HttpSession<Stream>::send_error_response(http::status status, std::string_view message) {
    auto res = std::make_shared<http::response<http::string_body>>(status, parser_->get().version());
    res->set(http::field::content_type, "text/plain");
    res->body() = std::string(message);
    res->prepare_payload();

    http::async_write(stream_, *res, [self = this->shared_from_this(), res](beast::error_code, std::size_t) {
        self->do_shutdown();
    });
}

template<class Stream>
bool HttpSession<Stream>::try_websocket_upgrade() {
    if (websocket::is_upgrade(parser_->get())) {
        std::string target(parser_->get().target());
        const WebSocketHandlers* handlers = websockets_.find_handler(target);

        if (handlers) {
            std::make_shared<WebSocketSession<Stream>>(
                std::move(stream_), *handlers, websockets_, target
            )->run(parser_->release());
            return true;
        }
    }
    return false;
}

template class HttpSession<beast::tcp_stream>;

} // namespace blaze
