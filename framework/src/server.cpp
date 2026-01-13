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

    // Write response from handler
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

    if (ec) {
        return;
    }

    if (!keep_alive) {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    do_read();
}