#include "server.h"
#include "listener.h"
#include "../http_session.h"
#include "../dispatcher.h"
#include "../signal_handler.h"
#include <blaze/logger.h>

#include <cstdio>
#include <stdexcept>
#include <utility>

namespace blaze {

Server::Server(
    Dispatcher& dispatcher,
    const Config& config)
    : dispatcher_(dispatcher),
      config_(config) {}

Server::~Server() {
    stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void Server::listen(const int port, int num_threads) {
    ServerState expected = ServerState::Stopped;
    if (!state_.compare_exchange_strong(expected, ServerState::Starting)) {
        throw std::logic_error("Server::listen: Server is already running or stopping.");
    }

    try {
        num_threads = resolve_thread_count(num_threads, config_.threads);

        const auto address = net::ip::make_address("0.0.0.0");
        const auto endpoint = tcp::endpoint{address, static_cast<unsigned short>(port)};

        listener_ = std::make_shared<Listener>(
            ioc_,
            endpoint,
            [this](tcp::socket socket) {
                if (state_.load() != ServerState::Running) {
                    boost::system::error_code ec;
                    socket.close(ec);
                    return;
                }
                auto session = std::make_shared<Session>(
                    dispatcher_,
                    config_,
                    beast::tcp_stream(std::move(socket)));
                session->run();
            });
        listener_->run();

        signals_ = std::make_unique<SignalHandler>(ioc_, [this](const int signal_number) {
            std::printf("[Blaze] Received signal %d, stopping...\n", signal_number);
            stop();
        });
        signals_->start();

        expected = ServerState::Starting;
        if (!state_.compare_exchange_strong(expected, ServerState::Running)) {
            listener_->stop();
            return;
        }

        run(num_threads);
    } catch (...) {
        state_.store(ServerState::Stopped);
        throw;
    }
}

void Server::stop() {
    ServerState expected = ServerState::Running;
    if (!state_.compare_exchange_strong(expected, ServerState::Stopping)) {
        if (state_.load() == ServerState::Starting) {
            state_.store(ServerState::Stopping);
        } else {
            return;
        }
    }

    if (signals_) {
        signals_->cancel();
    }
    if (listener_) {
        listener_->stop();
    }
    ioc_.stop();
    state_.store(ServerState::Stopped);
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
    state_.store(ServerState::Stopped);
}

} // namespace blaze
