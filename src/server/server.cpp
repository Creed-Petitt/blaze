#include "server.h"
#include "../http/http_session.h"
#include "../request/request_dispatcher.h"
#include "../signal/signal_handler.h"

#include <blaze/logger.h>
#include <blaze/websocket_registry.h>

#include <iostream>
#include <stdexcept>
#include <utility>

namespace blaze {

Listener::Listener(net::io_context& ioc, const tcp::endpoint& endpoint, AcceptHandler on_accept)
    : ioc_(ioc), acceptor_(ioc), on_accept_(std::move(on_accept)) {
    beast::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) throw std::runtime_error("Acceptor open failed: " + ec.message());

    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) throw std::runtime_error("Acceptor reuse_address failed: " + ec.message());

    acceptor_.bind(endpoint, ec);
    if (ec) throw std::runtime_error("Acceptor bind failed: " + ec.message());

    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) throw std::runtime_error("Acceptor listen failed: " + ec.message());
}

void Listener::run() {
    do_accept();
}

void Listener::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(const beast::error_code& ec, tcp::socket socket) {
    if (ec) {
        if (ec == net::error::operation_aborted) {
            return;
        }

        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument) {
            std::cerr << "accept error: " << ec.message() << std::endl;
        }
    } else {
        on_accept_(std::move(socket));
    }

    if (acceptor_.is_open()) {
        do_accept();
    }
}

void Listener::stop() {
    beast::error_code ec;
    acceptor_.close(ec);
}

Server::Server(RequestDispatcher& dispatcher, WebSocketRegistry& websockets, const Config& config)
    : dispatcher_(dispatcher), websockets_(websockets), config_(config) {}

Server::~Server() {
    stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void Server::listen(const int port, int num_threads) {
    num_threads = resolve_thread_count(num_threads, config_.threads);

    const auto address = net::ip::make_address("0.0.0.0");
    const auto endpoint = tcp::endpoint{address, static_cast<unsigned short>(port)};

    listener_ = std::make_shared<Listener>(
        ioc_,
        endpoint,
        [this](tcp::socket socket) {
            std::make_shared<Session>(
                dispatcher_,
                websockets_,
                config_,
                beast::tcp_stream(std::move(socket)))->run();
        });
    listener_->run();

    signals_ = std::make_unique<SignalHandler>(ioc_, [this](int signal_number) {
        std::cout << "[Blaze] Received signal " << signal_number << ", stopping..." << std::endl;
        stop();
    });
    signals_->start();

    run(num_threads);
}

void Server::stop() {
    if (signals_) {
        signals_->cancel();
    }
    if (listener_) {
        listener_->stop();
    }
    ioc_.stop();
}

void Server::spawn(Async<void> task) {
    boost::asio::co_spawn(ioc_.get_executor(), std::move(task), boost::asio::detached);
}

int Server::resolve_thread_count(const int requested, const int configured) {
    if (requested > 0) {
        return requested;
    }
    if (configured > 0) {
        return configured;
    }

    const auto hardware_threads = std::thread::hardware_concurrency();
    return hardware_threads == 0 ? 4 : static_cast<int>(hardware_threads);
}

void Server::run(const int num_threads) {
    threads_.clear();
    threads_.reserve(num_threads > 0 ? static_cast<size_t>(num_threads - 1) : 0);

    for (int i = 1; i < num_threads; ++i) {
        threads_.emplace_back([this] {
            ioc_.run();
        });
    }

    ioc_.run();

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}

} // namespace blaze
