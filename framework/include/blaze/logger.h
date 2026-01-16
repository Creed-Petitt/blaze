#ifndef HTTP_SERVER_LOGGER_H
#define HTTP_SERVER_LOGGER_H

#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace blaze {

class Logger {
private:
    std::ofstream access_log;
    std::ofstream error_log;
    
    // Async Queue
    std::queue<std::string> queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread worker_;
    std::atomic<bool> running_{true};

    static std::string get_timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_r(&now_time_t, &tm_buf);

        std::stringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    static void ensure_logs_directory() {
        struct stat st{};
        if (stat("../logs", &st) == -1) {
            mkdir("../logs", 0755);
        }
    }

    void process_queue() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

            while (!queue_.empty()) {
                std::string msg = std::move(queue_.front());
                queue_.pop();
                
                // Unlock during I/O
                lock.unlock();

                if (msg.starts_with("ERROR:")) {
                    if (error_log.is_open()) error_log << "[" << get_timestamp() << "] " << msg << "\n";
                } else {
                    if (access_log.is_open()) access_log << "[" << get_timestamp() << "] " << msg << "\n";
                }
                
                lock.lock();
            }
        }
    }

public:
    Logger() {
        ensure_logs_directory();
        access_log.open("../logs/access.log", std::ios::out | std::ios::app);
        error_log.open("../logs/error.log", std::ios::out | std::ios::app);

        worker_ = std::thread(&Logger::process_queue, this);
    }

    void log_access(std::string_view client_ip,
                   std::string_view method,
                   std::string_view path,
                   int status_code,
                   long long response_time_ms) {
        
        std::stringstream ss;
        ss << client_ip << " " << method << " " << path << " " 
           << status_code << " " << response_time_ms << "ms";
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(ss.str());
        cv_.notify_one();
    }

    void log_error(const std::string& message) {
        std::string msg = "ERROR: " + message;
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push(std::move(msg));
        cv_.notify_one();
    }

    ~Logger() {
        running_ = false;
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
        
        if (access_log.is_open()) access_log.close();
        if (error_log.is_open()) error_log.close();
    }
};

} // namespace blaze

#endif //HTTP_SERVER_LOGGER_H