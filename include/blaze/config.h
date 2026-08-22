#pragma once

#include "logger.h"

#include <cstdint>
#include <cstddef>
#include <chrono>

namespace blaze {

struct Config {
    uint16_t port{8080};
    int threads{0};
    size_t max_body_size{10 * 1024 * 1024}; // 10 MB
    std::chrono::seconds timeout_seconds{30};
    std::chrono::seconds shutdown_timeout{30};
    std::string log_path{"stdout"};
    LogLevel log_level{LogLevel::INFO};
    std::string server_name{"Blaze/1.0"};
};

class ConfigBuilder {
public:
    ConfigBuilder& port(uint16_t);
    ConfigBuilder& threads(int);
    ConfigBuilder& max_body_size(size_t);
    ConfigBuilder& timeout(std::chrono::seconds);
    ConfigBuilder& shutdown(std::chrono::seconds);
    ConfigBuilder& log_path(std::string_view);
    ConfigBuilder& log_level(LogLevel);
    ConfigBuilder& server_name(std::string_view);

    Config build();

private:
    Config config_;
};

} // namespace blaze
