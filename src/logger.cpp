#include <blaze/logger.h>

#include <chrono>
#include <format>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <utility>

namespace blaze {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    worker_ = std::thread(&Logger::process_queue, this);
}

Logger::~Logger() {
    running_ = false;
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
}

std::string Logger::get_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    
    localtime_r(&now_time_t, &tm_buf);

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::process_queue() {
    while (running_ || !queue_.empty()) {
        std::string msg;
        {
            std::unique_lock lock(queue_mutex_);
            cv_.wait(lock, [this] {
                return !queue_.empty() || !running_;
            });
            
            if (queue_.empty() && !running_) {
                break;
            }

            msg = std::move(queue_.front());
            queue_.pop();
        }

        std::stringstream output;
        output << "[" << get_timestamp() << "] " << msg << "\n";
        std::string out_str = output.str();

        std::lock_guard config_lock(config_mutex_);
        if (use_stdout_) {
            if (msg.contains("ERROR")) {
                std::cerr << out_str;
            } else {
                std::cout << out_str;
            }
        } else if (!log_path_.empty()) {
            if (!file_stream_.is_open()) {
                file_stream_.open(log_path_, std::ios::out | std::ios::app);
            }
            if (file_stream_.is_open()) {
                file_stream_ << out_str;
                if (msg.contains("ERROR")) {
                    file_stream_.flush();
                }
            }
        }
    }
}

void Logger::configure(const std::string& path) {
    std::lock_guard lock(config_mutex_);
    
    if (path == "/dev/null") {
        enabled_.store(false, std::memory_order_relaxed);
        return;
    }

    enabled_.store(true, std::memory_order_relaxed);

    if (path == "stdout" || path.empty()) {
        use_stdout_ = true;
        log_path_ = "";
        return;
    }

    use_stdout_ = false;
    
    // Close existing if the path changed
    if (log_path_ != path && file_stream_.is_open()) {
        file_stream_.close();
    }
    
    log_path_ = path;

    const std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
}

void Logger::log(const LogLevel level, const std::string_view message) {
    if (!enabled_.load(std::memory_order_relaxed) ||
        level < level_.load(std::memory_order_relaxed)) {
        return;
    }

    std::string level_str;
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO:  level_str = "INFO";  break;
        case LogLevel::WARN:  level_str = "WARN";  break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }

    {
        std::string msg = level_str + ": " + std::string(message);
        std::lock_guard lock(queue_mutex_);
        queue_.push(std::move(msg));
    }
    cv_.notify_one();
}

void Logger::log_access(
    const std::string_view client_ip,
    const std::string_view method,
    const std::string_view path,
    const int status_code,
    const long long response_time_ms) {
    if (!enabled_.load(std::memory_order_relaxed) ||
        LogLevel::INFO < level_.load(std::memory_order_relaxed)) {
        return;
    }

    auto msg = std::format(
        "ACCESS: {} {} {} {} {}ms",
        client_ip,
        method,
        path,
        status_code,
        response_time_ms);

    {
        std::lock_guard lock(queue_mutex_);
        queue_.push(std::move(msg));
    }
    cv_.notify_one();
}

void Logger::log_error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

} // namespace blaze
