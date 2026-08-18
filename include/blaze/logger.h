#pragma once

#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>

namespace blaze {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
private:
    std::ofstream file_stream_;
    bool use_stdout_{false};
    std::atomic<bool> enabled_{true};
    std::atomic<LogLevel> level_{LogLevel::INFO};
    
    // Config Mutex (protects file_stream_, use_stdout_, enabled_, level_)
    std::mutex config_mutex_;
    std::string log_path_;

    // Async Queue
    std::queue<std::string> queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> running_{true};

    static std::string get_timestamp();
    void process_queue();

    Logger();

public:
    ~Logger();
    
    // Global Access Point
    static Logger& instance();

    void configure(const std::string& path);
    void set_level(const LogLevel level) { level_.store(level, std::memory_order_relaxed); }
    LogLevel get_level() const { return level_.load(std::memory_order_relaxed); }

    void log(LogLevel level, std::string_view message);

    void log_access(
        std::string_view client_ip,
        std::string_view method,
        std::string_view path,
        int status_code,
        long long response_time_ms);

    void log_error(const std::string& message);
    
    void debug(const std::string& msg) {
        log(LogLevel::DEBUG, msg);
    }
    void info(const std::string& msg) {
        log(LogLevel::INFO, msg);
    }
    void warn(const std::string& msg) {
        log(LogLevel::WARN, msg);
    }
    void error(const std::string& msg) {
        log(LogLevel::ERROR, msg);
    }
};

inline void info(const std::string_view msg) {
    Logger::instance().log(LogLevel::INFO, msg);
}

inline void warn(const std::string_view msg) {
    Logger::instance().log(LogLevel::WARN, msg);
}

inline void error(const std::string_view msg) {
    Logger::instance().log(LogLevel::ERROR, msg);
}

inline void debug(const std::string_view msg) {
    Logger::instance().log(LogLevel::DEBUG, msg);
}

} // namespace blaze

// Macros for detailed error logging
#define BLAZE_LOG_ERROR(msg) \
    blaze::Logger::instance().log(blaze::LogLevel::ERROR, \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + msg)

#define BLAZE_LOG_WARN(msg) \
    blaze::Logger::instance().log(blaze::LogLevel::WARN, \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " + msg)
