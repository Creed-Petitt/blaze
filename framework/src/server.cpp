#include "server.h"
#include <blaze/app.h>
#include <blaze/request.h>
#include <blaze/response.h>
#include <blaze/exceptions.h>
#include <boost/beast/http/file_body.hpp>
#include <iostream>

namespace blaze {

// Helper to handle stream-specific operations
template<class Stream>
struct StreamTraits {
    static void shutdown(Stream& stream) {
        beast::error_code ec;
        if constexpr (std::is_same_v<Stream, beast::tcp_stream>) {
            stream.socket().shutdown(tcp::socket::shutdown_send, ec);
        } else {
            beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_send, ec);
        }
    }
    
    static tcp::socket& get_socket(Stream& stream) {
        if constexpr (std::is_same_v<Stream, beast::tcp_stream>) {
            return stream.socket();
        } else {
            return beast::get_lowest_layer(stream).socket();
        }
    }
};

namespace {

    Request from_beast(http::request<http::string_body>&& req) {
        Request blaze_req;
        blaze_req.method = std::string(req.method_string());
        blaze_req.set_target(std::string_view(req.target().data(), req.target().size()));
        blaze_req.body = std::move(req.body());
        blaze_req.set_fields(std::move(req.base()));
        return blaze_req;
    }

    template <typename Stream>
    boost::asio::awaitable<void> send_error(Stream& stream, http::status status, std::string_view msg, unsigned version) {
        http::response<http::string_body> res{status, version};
        res.set(http::field::content_type, "application/json");
        res.body() = "{\"error\": \"" + std::string(msg) + "\"}";
        res.prepare_payload();
        co_await http::async_write(stream, res, net::use_awaitable);
    }

    template <typename Stream, typename SessionPtr>
    boost::asio::awaitable<void> handle_session(
        Stream& stream,
        SessionPtr self,
        App& app,
        Request req,
        std::string client_ip,
        bool keep_alive
    ) {
        Response blaze_res;
        bool error_occurred = false;
        std::optional<std::pair<http::status, std::string>> pending_error;

        try {
            blaze_res = co_await app.handle_request(req, client_ip, keep_alive);
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
                co_await send_error(stream, pending_error->first, pending_error->second, 11);
            } catch (...) {
                error_occurred = true;
            }
        } else if (blaze_res.is_file()) {
            // Handle file streaming
            beast::error_code ec;
            http::file_body::value_type body;
            body.open(blaze_res.get_file_path().c_str(), beast::file_mode::scan, ec);

            if (ec) {
                co_await send_error(stream, http::status::not_found, "File not found", 11);
            } else {
                auto const size = body.size();
                http::response<http::file_body> res{
                    std::piecewise_construct,
                    std::make_tuple(std::move(body)),
                    std::make_tuple(static_cast<http::status>(blaze_res.get_status()), 11)
                };

                // Copy headers from blaze_res to our file response
                auto& beast_res = blaze_res.get_beast_response();
                for (auto const& field : beast_res) {
                    res.set(field.name(), field.value());
                }
                
                res.content_length(size);
                res.keep_alive(keep_alive);
                res.prepare_payload();

                try {
                    co_await http::async_write(stream, res, net::use_awaitable);
                } catch (...) {
                    error_occurred = true;
                }
            }
        } else {
            // Handle standard string response
            auto& beast_res = blaze_res.get_beast_response();
            beast_res.keep_alive(keep_alive);
            beast_res.prepare_payload();

            try {
                co_await http::async_write(stream, beast_res, net::use_awaitable);
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
}

template<class Stream>
WebSocketSession<Stream>::WebSocketSession(Stream&& stream, const WebSocketHandlers& handlers, App& app, std::string target)
    : ws_(std::move(stream)), handlers_(handlers), app_(app), target_(std::move(target)) {}

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

    // Register with App for automatic broadcasting
    app_._register_ws(target_, this->shared_from_this());

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
void WebSocketSession<Stream>::on_read(beast::error_code ec, std::size_t bytes_transferred) {
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
        handlers_.on_message(std::static_pointer_cast<WebSocket>(this->shared_from_this()), beast::buffers_to_string(buffer_.data()));
    }
    
    buffer_.consume(buffer_.size());
    do_read();
}

template<class Stream>
void WebSocketSession<Stream>::send(std::string message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
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

    std::lock_guard<std::mutex> lock(queue_mutex_);
    write_queue_.pop();

    if(!write_queue_.empty()) {
        do_write();
    }
}

template<class Stream>
void WebSocketSession<Stream>::close() {
    ws_.async_close(websocket::close_code::normal,
        [](beast::error_code ec) {
            if(ec) std::cerr << "WS Close Error: " << ec.message() << "\n";
        });
}

// Explicit Instantiation
template class WebSocketSession<beast::tcp_stream>;
template class WebSocketSession<ssl::stream<beast::tcp_stream>>; 

template<class Stream>
template<typename... Args>
HttpSession<Stream>::HttpSession(App& app, Args&&... args)
    : stream_(std::forward<Args>(args)...), app_(app) {}

template<class Stream>
void HttpSession<Stream>::run() {
    if constexpr (std::is_same_v<Stream, beast::tcp_stream>) {
        net::dispatch(
            stream_.get_executor(),
            beast::bind_front_handler(&HttpSession::do_read, this->shared_from_this()));
    } else {
        // SSL Handshake
        beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
        stream_.async_handshake(
            ssl::stream_base::server,
            beast::bind_front_handler(
                [self = this->shared_from_this()](beast::error_code ec) {
                    if(ec) {
                        std::cerr << "SSL handshake error: " << ec.message() << "\n";
                        return;
                    }
                    self->do_read();
                }));
    }
}

template<class Stream>
void HttpSession<Stream>::do_read() {
    parser_.emplace();
    parser_->body_limit(app_.get_config().max_body_size);
    
    beast::get_lowest_layer(stream_).expires_after(
        std::chrono::seconds(app_.get_config().timeout_seconds)
    );

    http::async_read(stream_, buffer_, *parser_,
        beast::bind_front_handler(&HttpSession::on_read, this->shared_from_this()));
}

template<class Stream>
void HttpSession<Stream>::on_read(beast::error_code ec, std::size_t bytes_transferred) {
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

        if (ec != net::error::connection_reset && ec != net::error::eof && ec != beast::error::timeout && ec != ssl::error::stream_truncated) {
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
    bool keep_alive = beast_req.keep_alive();
    
    std::string client_ip = get_client_ip();

    boost::asio::co_spawn(
        stream_.get_executor(),
        handle_session(
            stream_, 
            this->shared_from_this(), 
            app_, 
            from_beast(std::move(beast_req)), 
            client_ip, 
            keep_alive
        ),
        boost::asio::detached
    );
}

template<class Stream>
void HttpSession<Stream>::on_write(bool keep_alive, beast::error_code ec, std::size_t bytes_transferred) {
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
    beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));
    if constexpr (std::is_same_v<Stream, beast::tcp_stream>) {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    } else {
        stream_.async_shutdown(
            beast::bind_front_handler(
                [self = this->shared_from_this()](beast::error_code ec) {
                    boost::ignore_unused(ec);
                }));
    }
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
        const WebSocketHandlers* handlers = app_.get_ws_handler(target);
        
        if (handlers) {
            std::make_shared<WebSocketSession<Stream>>(
                std::move(stream_), *handlers, app_, target
            )->run(parser_->release());
            return true;
        }
    }
    return false;
}

// Explicit Instantiations
template class HttpSession<beast::tcp_stream>;
template class HttpSession<ssl::stream<beast::tcp_stream>>;

Listener::Listener(net::io_context& ioc, const tcp::endpoint &endpoint, App& app)
    : ioc_(ioc), acceptor_(ioc), app_(app) {
    beast::error_code ec;
    
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) throw std::runtime_error("Acceptor open failed: " + ec.message());

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    
    acceptor_.bind(endpoint, ec);
    if(ec) throw std::runtime_error("Acceptor bind failed: " + ec.message());

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) throw std::runtime_error("Acceptor listen failed: " + ec.message());
}

void Listener::run() { do_accept(); }

void Listener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        if (ec == net::error::operation_aborted)
            return; // Shutdown in progress
        
        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument)
            std::cerr << "accept error: " << ec.message() << std::endl;
    } else {
        std::make_shared<Session>(app_, std::move(socket))->run();
    }
    do_accept();
}

void Listener::stop() {
    beast::error_code ec;
    acceptor_.close(ec);
}

SslListener::SslListener(net::io_context& ioc, ssl::context& ctx, const tcp::endpoint &endpoint, App& app)
    : ioc_(ioc), ctx_(ctx), acceptor_(ioc), app_(app) {
    beast::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) throw std::runtime_error("SSL Acceptor open failed: " + ec.message());
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) throw std::runtime_error("SSL Acceptor set_option failed: " + ec.message());
    acceptor_.bind(endpoint, ec);
    if(ec) throw std::runtime_error("SSL Acceptor bind failed: " + ec.message());
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) throw std::runtime_error("SSL Acceptor listen failed: " + ec.message());
}

void SslListener::run() { do_accept(); }

void SslListener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&SslListener::on_accept, shared_from_this()));
}

void SslListener::on_accept(const beast::error_code ec, tcp::socket socket) {
    if(ec) {
        if (ec == net::error::operation_aborted)
            return; // Shutdown in progress

        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument)
            std::cerr << "accept error: " << ec.message() << std::endl;
    } else {
        std::make_shared<SslSession>(app_, std::move(socket), ctx_)->run();
    }
    do_accept();
}

void SslListener::stop() {
    beast::error_code ec;
    acceptor_.close(ec);
}

} // namespace blaze
