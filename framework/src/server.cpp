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