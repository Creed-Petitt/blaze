#include "websocket_session.h"

#include <iostream>
#include <utility>

namespace blaze {

template<class Stream>
WebSocketSession<Stream>::WebSocketSession(
    Stream&& stream,
    const WebSocketHandlers& handlers,
    WebSocketRegistry& registry,
    std::string target)
    : ws_(std::move(stream)), handlers_(handlers), registry_(registry), target_(std::move(target)) {}

template<class Stream>
void WebSocketSession<Stream>::run(http::request<http::string_body> req) {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    ws_.async_accept(req,
        beast::bind_front_handler(
            &WebSocketSession::on_accept,
            this->shared_from_this()));
}

template<class Stream>
void WebSocketSession<Stream>::on_accept(beast::error_code ec) {
    if(ec) {
        std::cerr << "WS Accept Error: " << ec.message() << "\n";
        return;
    }

    registry_.register_session(target_, this->shared_from_this());

    if (handlers_.on_open) {
        handlers_.on_open(std::static_pointer_cast<WebSocket>(this->shared_from_this()));
    }

    do_read();
}

template<class Stream>
void WebSocketSession<Stream>::do_read() {
    ws_.async_read(buffer_,
        beast::bind_front_handler(
            &WebSocketSession::on_read,
            this->shared_from_this()));
}

template<class Stream>
void WebSocketSession<Stream>::on_read(const beast::error_code& ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec == websocket::error::closed) {
        if (handlers_.on_close) handlers_.on_close(std::static_pointer_cast<WebSocket>(this->shared_from_this()));
        return;
    }

    if(ec) {
        if (ec != net::error::operation_aborted &&
            ec != net::error::connection_reset &&
            ec != beast::error::timeout &&
            ec.value() != 107) {
            std::cerr << "[WS] Session Error: " << ec.message() << "\n";
        }
        return;
    }

    if (handlers_.on_message) {
        handlers_.on_message(
            std::static_pointer_cast<WebSocket>(this->shared_from_this()),
            beast::buffers_to_string(buffer_.data()));
    }

    buffer_.consume(buffer_.size());
    do_read();
}

template<class Stream>
void WebSocketSession<Stream>::send(std::string message) {
    std::lock_guard lock(queue_mutex_);
    write_queue_.push(std::move(message));

    if(write_queue_.size() > 1) {
        return;
    }

    do_write();
}

template<class Stream>
void WebSocketSession<Stream>::do_write() {
    ws_.text(true);
    ws_.async_write(
        net::buffer(write_queue_.front()),
        beast::bind_front_handler(
            &WebSocketSession::on_write,
            this->shared_from_this()));
}

template<class Stream>
void WebSocketSession<Stream>::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec) {
        std::cerr << "WS Write Error: " << ec.message() << "\n";
        return;
    }

    std::lock_guard lock(queue_mutex_);
    write_queue_.pop();

    if(!write_queue_.empty()) {
        do_write();
    }
}

template<class Stream>
void WebSocketSession<Stream>::close() {
    ws_.async_close(websocket::close_code::normal,
        [](const beast::error_code& ec) {
            if(ec) std::cerr << "WS Close Error: " << ec.message() << "\n";
        });
}

template class WebSocketSession<beast::tcp_stream>;

} // namespace blaze
