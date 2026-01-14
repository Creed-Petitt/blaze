#include "server.h"
#include <blaze/app.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <iostream>

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
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};

    // Set the timeout
    beast::get_lowest_layer(stream_).
        expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream_, buffer_, req_,
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
        std::cerr << "read error: " << ec.message() << std::endl;
        return;
    }

    Request blaze_req;
    blaze_req.method = std::string(req_.method_string());
    blaze_req.set_target({req_.target().data(), req_.target().size()});
    blaze_req.body = req_.body();

    for(auto const& field : req_) {
        blaze_req.add_header(
            {field.name_string().data(), field.name_string().size()},
            {field.value().data(), field.value().size()}
        );
    }

    const std::string client_ip = stream_.socket().remote_endpoint().address().to_string();
    std::string response_str = app_.handle_request(blaze_req, client_ip, req_.keep_alive());

    const auto response_ptr = std::make_shared<std::string>(std::move(response_str));

    // Write response from App handler
    net::async_write(
        stream_,
        net::buffer(*response_ptr),
        [self = shared_from_this(), response_ptr](const beast::error_code code, const std::size_t bytes){
                self->on_write(self->req_.keep_alive(), code, bytes);
        }
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
    req_ = {};

    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

    http::async_read(stream_, buffer_, req_,
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
        std::cerr << "SSL read error: " << ec.message() << std::endl;
        return;
    }

    Request blaze_req;
    blaze_req.method = std::string(req_.method_string());
    blaze_req.set_target({req_.target().data(), req_.target().size()});
    blaze_req.body = req_.body();

    for(auto const& field : req_) {
        blaze_req.add_header(
            {field.name_string().data(), field.name_string().size()},
            {field.value().data(), field.value().size()}
        );
    }

    const std::string client_ip = beast::get_lowest_layer(stream_).socket().remote_endpoint().address().to_string();
    std::string response_str = app_.handle_request(blaze_req, client_ip, req_.keep_alive());

    const auto response_ptr = std::make_shared<std::string>(std::move(response_str));

    net::async_write(
        stream_,
        net::buffer(*response_ptr),
        [self = shared_from_this(), response_ptr](const beast::error_code code, const std::size_t bytes){
                self->on_write(self->req_.keep_alive(), code, bytes);
        }
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

void SslSession::on_shutdown(const beast::error_code ec) {
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
    } else {
        std::make_shared<SslSession>(std::move(socket), ctx_, app_)->run();
    }
    do_accept();
}
