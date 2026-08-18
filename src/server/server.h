#pragma once

#include <blaze/config.h>
#include <blaze/router.h>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>

#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace blaze {

class RequestDispatcher;
class SignalHandler;
class WebSocketRegistry;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    using AcceptHandler = std::function<void(tcp::socket)>;

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    AcceptHandler on_accept_;

public:
    Listener(net::io_context& ioc, const tcp::endpoint &endpoint, AcceptHandler on_accept);
    void run();
    void do_accept();
    void on_accept(const beast::error_code& ec, tcp::socket socket);
    void stop();
};

class Server {
    RequestDispatcher& dispatcher_;
    WebSocketRegistry& websockets_;
    const Config& config_;
    net::io_context ioc_;
    std::shared_ptr<Listener> listener_;
    std::unique_ptr<SignalHandler> signals_;
    std::vector<std::thread> threads_;

public:
    Server(RequestDispatcher& dispatcher, WebSocketRegistry& websockets, const Config& config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void listen(int port, int num_threads);
    void stop();
    void spawn(Async<void> task);
    net::io_context& engine() { return ioc_; }

private:
    static int resolve_thread_count(int requested, int configured);
    void run(int num_threads);
};

} // namespace blaze
