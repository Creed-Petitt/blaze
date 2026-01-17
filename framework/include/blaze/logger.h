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
#include <filesystem>

namespace blaze {

class Logger {
private:
    std::ofstream file_stream_;
    bool use_stdout_{false};
    
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

    void process_queue() {
        while (running_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

            while (!queue_.empty()) {
                std::string msg = std::move(queue_.front());
                queue_.pop();
                
                // Unlock during I/O
                lock.unlock();

                std::stringstream output;
                output << "[" << get_timestamp() << "] " << msg << "\n";

                if (use_stdout_) {
                    if (msg.starts_with("ERROR:")) {
                        std::cerr << output.str();
                    } else {
                        std::cout << output.str();
                    }
                } else if (file_stream_.is_open()) {
                    file_stream_ << output.str();
                    file_stream_.flush();
                }
                
                lock.lock();
            }
        }
    }

public:
    Logger() {
        // Start the worker immediately
        worker_ = std::thread(&Logger::process_queue, this);
    }

    // Called when Server Starts
    void configure(const std::string& path) {
        if (path == "stdout" || path.empty()) {
            use_stdout_ = true;
            return;
        }

        use_stdout_ = false;
        
        // Ensure directory exists
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        // Open file (Append Mode)
        if (file_stream_.is_open()) file_stream_.close();
        file_stream_.open(path, std::ios::out | std::ios::app);
    }

    void log_access(std::string_view client_ip,
                   std::string_view method,
                   std::string_view path,
                   int status_code,
                   long long response_time_ms) {
        
        std::stringstream ss;
        // GREEN/RESET ANSI codes could go here if stdout
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
        if (file_stream_.is_open()) file_stream_.close();
    }
};

} // namespace blaze

#endif //HTTP_SERVER_LOGGER_H
