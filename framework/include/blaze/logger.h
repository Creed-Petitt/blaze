#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H

#include <string>
#include <string_view>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

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
    bool enabled_{true};
    LogLevel level_{LogLevel::INFO};
    
    // Async Queue
    std::queue<std::string> queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> running_{true};

    static std::string get_timestamp();
    void process_queue();

public:
    Logger();
    ~Logger();

    // Configuration
    void configure(const std::string& path);
    void set_level(LogLevel level) { level_ = level; }
    LogLevel get_level() const { return level_; }

    void log(LogLevel level, std::string_view message);

    void log_access(std::string_view client_ip,
                   std::string_view method,
                   std::string_view path,
                   int status_code,
                   long long response_time_ms);

    void log_error(const std::string& message);
    
    // Convenience methods
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg) { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg) { log(LogLevel::WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
};

} // namespace blaze

#endif //HTTP_SERVER_LOGGER_H
