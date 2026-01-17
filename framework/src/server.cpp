#include "server.h"
#include <blaze/app.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <iostream>

namespace blaze {

namespace {

    // Converts Beast Request -> Blaze Request
    Request from_beast(http::request<http::string_body>&& req) {
        Request blaze_req;
        blaze_req.method = {req.method_string().data(), req.method_string().size()};
        blaze_req.set_target({req.target().data(), req.target().size()});
        blaze_req.body = std::move(req.body());
        blaze_req.set_fields(req.base());
        return blaze_req;
    }

    // Shared logic for HTTP and HTTPS
    template <typename Stream, typename SessionPtr>
    boost::asio::awaitable<void> handle_session(
        Stream& stream,
        SessionPtr self,
        App& app,
        Request req,
        std::string client_ip,
        bool keep_alive
    ) {
        try {
            // Business Logic
            std::string response_str = co_await app.handle_request(req, client_ip, keep_alive);
            
            // Write Response
            co_await boost::asio::async_write(
                stream,
                boost::asio::buffer(response_str),
                boost::asio::use_awaitable
            );

            if (!keep_alive) {
                beast::error_code ec;

                if constexpr (std::is_same_v<Stream, beast::tcp_stream>) {
                    stream.socket().shutdown(tcp::socket::shutdown_send, ec);
                } else {
                    // For SSL, we typically do async_shutdown in the session,
                    // but here we just signal completion.
                    // The caller (Session) handles the loop via callbacks usually,
                    // but since we are in a coroutine, we can just return.
                    // Ideally SSL shutdown is a separate async op.
                }
            } else {
                // Read Next Request
                self->do_read();
            }

        } catch (const std::exception& e) {
            std::cerr << "Async Handler Error: " << e.what() << "\n";
            
            // 500 Recovery
            try {
                static const std::string ERROR_500 = 
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 21\r\n"
                    "Connection: close\r\n\r\n"
                    "Internal Server Error";
                    
                co_await boost::asio::async_write(
                    stream,
                    boost::asio::buffer(ERROR_500),
                    boost::asio::use_awaitable
                );
            } catch (...) {}
        }
    }
}

Session::Session(tcp::socket&& socket, App& app)
    : stream_(std::move(socket)), app_(app) {}

void Session::run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&Session::do_read, shared_from_this()));
}

void Session::do_read() {
    parser_.emplace();
    parser_->body_limit(app_.get_config().max_body_size);
    
    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(app_.get_config().timeout_seconds)
    );

    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(&Session::on_read, shared_from_this()));
}

void Session::on_read(beast::error_code ec, const std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }
    if (ec) {
        if (ec != net::error::connection_reset && ec != net::error::eof && ec != beast::error::timeout) {
            std::cerr << "read error: " << ec.message() << std::endl;
        }
        return;
    }

    // The Dispatcher
    auto beast_req = parser_->release();
    bool keep_alive = beast_req.keep_alive();
    std::string client_ip = stream_.socket().remote_endpoint().address().to_string();

    boost::asio::co_spawn(
        stream_.get_executor(),
        handle_session(
            stream_, 
            shared_from_this(), 
            app_, 
            from_beast(std::move(beast_req)), 
            client_ip, 
            keep_alive
        ),
        boost::asio::detached
    );
}

void Session::on_write(const bool keep_alive, beast::error_code ec, const std::size_t bytes_transferred) {
    // Legacy callback - mostly replaced by coroutine flow, 
    // but kept if needed for mixed async models.
    boost::ignore_unused(bytes_transferred);
    if (ec) return;
    if (!keep_alive) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }
    do_read();
}

SslSession::SslSession(tcp::socket&& socket, ssl::context& ctx, App& app)
    : stream_(std::move(socket), ctx), app_(app) {}

void SslSession::run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&SslSession::on_run, shared_from_this()));
}

void SslSession::on_run() {
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    stream_.async_handshake(
        ssl::stream_base::server,
        beast::bind_front_handler(&SslSession::on_handshake, shared_from_this()));
}

void SslSession::on_handshake(beast::error_code ec) {
    if(ec) {
        std::cerr << "SSL handshake error: " << ec.message() << std::endl;
        return;
    }
    do_read();
}

void SslSession::do_read() {
    parser_.emplace();
    parser_->body_limit(app_.get_config().max_body_size);

    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(app_.get_config().timeout_seconds)
    );

    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(&SslSession::on_read, shared_from_this()));
}

void SslSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec == http::error::end_of_stream) {
        do_shutdown();
        return;
    }
    if(ec) {
        if (ec != net::error::connection_reset && 
            ec != net::error::eof && 
            ec != ssl::error::stream_truncated && 
            ec != beast::error::timeout) {
            std::cerr << "SSL read error: " << ec.message() << std::endl;
        }
        return;
    }

    auto beast_req = parser_->release();
    bool keep_alive = beast_req.keep_alive();
    std::string client_ip = beast::get_lowest_layer(stream_).socket().remote_endpoint().address().to_string();

    boost::asio::co_spawn(
        stream_.get_executor(),
        handle_session(
            stream_, 
            shared_from_this(), 
            app_, 
            from_beast(std::move(beast_req)), 
            client_ip, 
            keep_alive
        ),
        boost::asio::detached
    );
}

void SslSession::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if(ec) return;
    if(!keep_alive) {
        do_shutdown();
        return;
    }
    do_read();
}

void SslSession::do_shutdown() {
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    stream_.async_shutdown(
        beast::bind_front_handler(&SslSession::on_shutdown, shared_from_this()));
}

void SslSession::on_shutdown(beast::error_code ec) {
    if(ec && ec != net::ssl::error::stream_truncated) {
         // Log shutdown error if needed
    }
}

Listener::Listener(net::io_context& ioc, const tcp::endpoint &endpoint, App& app)
    : ioc_(ioc), acceptor_(ioc), app_(app) {
    beast::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) { std::cerr << "Open error: " << ec.message() << std::endl; return; }
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) { std::cerr << "Set option error: " << ec.message() << std::endl; return; }
    acceptor_.bind(endpoint, ec);
    if(ec) { std::cerr << "Bind error: " << ec.message() << std::endl; return; }
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) { std::cerr << "Listen error: " << ec.message() << std::endl; return; }
}

void Listener::run() { do_accept(); }

void Listener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument)
            std::cerr << "accept error: " << ec.message() << std::endl;
    } else {
        std::make_shared<Session>(std::move(socket), app_)->run();
    }
    do_accept();
}

SslListener::SslListener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint &endpoint, App& app)
    : ioc_(ioc), ctx_(ctx), acceptor_(ioc), app_(app) {
    beast::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) { std::cerr << "Open error: " << ec.message() << std::endl; return; }
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) { std::cerr << "Set option error: " << ec.message() << std::endl; return; }
    acceptor_.bind(endpoint, ec);
    if(ec) { std::cerr << "Bind error: " << ec.message() << std::endl; return; }
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) { std::cerr << "Listen error: " << ec.message() << std::endl; return; }
}

void SslListener::run() { do_accept(); }

void SslListener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&SslListener::on_accept, shared_from_this()));
}

void SslListener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument)
            std::cerr << "accept error: " << ec.message() << std::endl;
    } else {
        std::make_shared<SslSession>(std::move(socket), ctx_, app_)->run();
    }
    do_accept();
}

} // namespace blaze