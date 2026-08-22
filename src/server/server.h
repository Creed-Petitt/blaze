#pragma once

#include <blaze/config.h>
#include <blaze/router.h>

#include <boost/asio.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace blaze {

namespace net = boost::asio;

class Listener;
class Dispatcher;
class SignalHandler;

enum class ServerState {
    Stopped,
    Starting,
    Running,
    Stopping
};

class Server {
    Dispatcher& dispatcher_;
    const Config& config_;
    net::io_context ioc_;
    std::shared_ptr<Listener> listener_;
    std::unique_ptr<SignalHandler> signals_;
    std::vector<std::thread> threads_;

    std::atomic<ServerState> state_{ServerState::Stopped};

public:
    Server(
        Dispatcher& dispatcher,
        const Config& config);

    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void listen(int port, int num_threads);
    void stop();
    void spawn(Async<void> task);
    net::io_context& engine() { return ioc_; }

    ServerState state() const { return state_.load(); }

private:
    static int resolve_thread_count(int requested, int configured);
    void run(int num_threads);
};

} // namespace blaze
