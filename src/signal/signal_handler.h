#pragma once

#include <boost/asio.hpp>

#include <functional>
#include <memory>

namespace net = boost::asio;

namespace blaze {

class SignalHandler {
    net::signal_set signals_;
    std::function<void(int)> on_signal_;

public:
    SignalHandler(net::io_context& ioc, std::function<void(int)> on_signal);

    void start();
    void cancel();
};

} // namespace blaze
