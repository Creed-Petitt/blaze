#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>

#include <functional>
#include <memory>

namespace blaze {

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    using AcceptHandler = std::function<void(tcp::socket)>;

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    AcceptHandler on_accept_;

public:
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, AcceptHandler on_accept);

    void run();
    void stop();

private:
    void do_accept();
    void on_accept(const beast::error_code& ec, tcp::socket socket);
};

} // namespace blaze
