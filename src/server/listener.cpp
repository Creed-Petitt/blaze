#include "listener.h"

#include <iostream>
#include <stdexcept>
#include <utility>

namespace blaze {

Listener::Listener(
    net::io_context& ioc,
    const tcp::endpoint& endpoint,
    AcceptHandler on_accept)
    : ioc_(ioc), acceptor_(ioc), on_accept_(std::move(on_accept))
{
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

void Listener::run()
{
    do_accept();
}

void Listener::do_accept()
{
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(const beast::error_code& ec, tcp::socket socket)
{
    if (ec) {
        if (ec == net::error::operation_aborted)
        {
            return;
        }

        if (ec != net::error::bad_descriptor && ec != net::error::invalid_argument)
        {
            std::cerr << "accept error: " << ec.message() << std::endl;
        }
    } else
    {
        on_accept_(std::move(socket));
    }

    if (acceptor_.is_open()) {
        do_accept();
    }
}

void Listener::stop()
{
    beast::error_code ec;
    acceptor_.close(ec);
}

} // namespace blaze
