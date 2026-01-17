#include "server.h"
#include <blaze/app.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <iostream>

namespace blaze {

Session::Session(tcp::socket&& socket, App& app)
    : stream_(std::move(socket)), app_(app) {}

void Session::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session.
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(
            &Session::do_read,
            shared_from_this()));
}

void Session::do_read() {
    // Construct a new parser for each request
    parser_.emplace();
    
    // Apply Limits from App Config
    parser_->body_limit(app_.get_config().max_body_size);

    // Set the timeout
    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(app_.get_config().timeout_seconds)
    );

    // Read a request using the parser
    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(
            &Session::on_read,
            shared_from_this()));
}

void Session::on_read(beast::error_code ec, const std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means client closed connections
    if (ec == http::error::end_of_stream) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec) {
        if (ec != net::error::connection_reset && 
            ec != net::error::eof && 
            ec != beast::error::timeout) {
            std::cerr << "read error: " << ec.message() << std::endl;
        }
        return;
    }

    // Extract the request from the parser
    auto req = parser_->release();

    Request blaze_req;
    blaze_req.method = {req.method_string().data(), req.method_string().size()};
    blaze_req.set_target({req.target().data(), req.target().size()});
    blaze_req.body = req.body();
    blaze_req.set_fields(req.base());

    const std::string client_ip = stream_.socket().remote_endpoint().address().to_string();
    bool keep_alive = req.keep_alive();
    
    // Spawn async handler
    boost::asio::co_spawn(
        stream_.get_executor(),
        [self = shared_from_this(), req = std::move(blaze_req), client_ip, keep_alive]() mutable -> boost::asio::awaitable<void> {
            try {
                std::string response_str = co_await self->app_.handle_request(req, client_ip, keep_alive);
                
                // Write response
                co_await boost::asio::async_write(
                    self->stream_,
                    boost::asio::buffer(response_str),
                    boost::asio::use_awaitable
                );

                if (!keep_alive) {
                    beast::error_code ec;
                    self->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
                } else {
                    self->do_read();
                }
            } catch (const std::exception& e) {
                std::cerr << "Async Handler Error: " << e.what() << "\n";
                // Attempt to send a raw 500 response
                try {
                    std::string error_res = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\nConnection: close\r\n\r\nInternal Server Error";
                    co_await boost::asio::async_write(
                        self->stream_,
                        boost::asio::buffer(error_res),
                        boost::asio::use_awaitable
                    );
                } catch (...) {}
            }
        },
        boost::asio::detached
    );
}

void Session::on_write(const bool keep_alive, beast::error_code ec, const std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) return;

    if (!keep_alive) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    do_read();
}

Listener::Listener(net::io_context& ioc, const tcp::endpoint &endpoint, App& app)
    : ioc_(ioc), acceptor_(ioc), app_(app) {

    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
        std::cerr << "Open error: " << ec.message() << std::endl;
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) {
        std::cerr << "Set option error: " << ec.message() << std::endl;
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec) {
        std::cerr << "Bind error: " << ec.message() << std::endl;
        return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) {
        std::cerr << "Listen error: " << ec.message() << std::endl;
        return;
    }
}

void Listener::run() {
    do_accept();
}

void Listener::do_accept() {
// The new connection gets its own strand
    acceptor_.async_accept(
    net::make_strand(ioc_),
        beast::bind_front_handler(
            &Listener::on_accept,
            shared_from_this())
    );
}

void Listener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        std::cerr << "accept error: " << ec.message() << std::endl;
        if (ec == net::error::bad_descriptor || ec == net::error::invalid_argument) return;
    } else {
    // Create the session and run it
        std::make_shared<Session>(std::move(socket), app_)->run();
    }

        // Accept another connection
    do_accept();
}

// SSL Session Implementation

SslSession::SslSession(tcp::socket&& socket, ssl::context& ctx, App& app)
    : stream_(std::move(socket), ctx), app_(app) {}

void SslSession::run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(
            &SslSession::on_run,
            shared_from_this()));
}

void SslSession::on_run() {
    // Set the timeout.
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    stream_.async_handshake(
        ssl::stream_base::server,
        beast::bind_front_handler(
            &SslSession::on_handshake,
            shared_from_this()));
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
        beast::bind_front_handler(
            &SslSession::on_read,
            shared_from_this()));
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

    auto req = parser_->release();

    Request blaze_req;
    blaze_req.method = {req.method_string().data(), req.method_string().size()};
    blaze_req.set_target({req.target().data(), req.target().size()});
    blaze_req.body = req.body();
    blaze_req.set_fields(req.base());

    const std::string client_ip = beast::get_lowest_layer(stream_).socket().remote_endpoint().address().to_string();
    bool keep_alive = req.keep_alive();
    
    // Spawn async handler
    boost::asio::co_spawn(
        stream_.get_executor(),
        [self = shared_from_this(), req = std::move(blaze_req), client_ip, keep_alive]() mutable -> boost::asio::awaitable<void> {
            try {
                std::string response_str = co_await self->app_.handle_request(req, client_ip, keep_alive);
                
                const auto response_ptr = std::make_shared<std::string>(std::move(response_str));

                // Write response
                co_await net::async_write(
                    self->stream_,
                    net::buffer(*response_ptr),
                    boost::asio::use_awaitable
                );

                self->on_write(keep_alive, {}, 0); // Manually trigger next step
            } catch (const std::exception& e) {
                std::cerr << "Async Handler Error: " << e.what() << "\n";
                // Attempt to send a raw 500 response
                try {
                    std::string error_res = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\nConnection: close\r\n\r\nInternal Server Error";
                    co_await boost::asio::async_write(
                        self->stream_,
                        boost::asio::buffer(error_res),
                        boost::asio::use_awaitable
                    );
                } catch (...) {}
            }
        },
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
        beast::bind_front_handler(
            &SslSession::on_shutdown,
            shared_from_this()));
}

void SslSession::on_shutdown(beast::error_code ec) {
    if(ec && ec != net::ssl::error::stream_truncated) {
         // Log shutdown error if needed
    }
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

void SslListener::run() {
    do_accept();
}

void SslListener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &SslListener::on_accept,
            shared_from_this()));
}

void SslListener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        std::cerr << "accept error: " << ec.message() << std::endl;
        if (ec == net::error::bad_descriptor || ec == net::error::invalid_argument) return;
    } else {
        std::make_shared<SslSession>(std::move(socket), ctx_, app_)->run();
    }
    do_accept();
}

} // namespace blaze
