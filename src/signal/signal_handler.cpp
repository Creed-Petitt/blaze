#include "signal_handler.h"

#include <iostream>

namespace blaze {

SignalHandler::SignalHandler(net::io_context& ioc, std::function<void(int)> on_signal)
    : signals_(ioc, SIGINT, SIGTERM), on_signal_(std::move(on_signal)) {}

void SignalHandler::start() {
    signals_.async_wait([this](const boost::system::error_code& ec, const int signal_number) {
        if (ec == net::error::operation_aborted) return;
        if (ec) {
            std::cerr << "Signal error: " << ec.message() << std::endl;
            return;
        }
        on_signal_(signal_number);
    });
}

void SignalHandler::cancel() {
    signals_.cancel();
}

} // namespace blaze
