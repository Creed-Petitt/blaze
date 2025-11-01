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

class Logger {
private:
    std::ofstream access_log;
    std::ofstream error_log;
    std::mutex log_mutex;

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

public:
    Logger() {
        ensure_logs_directory();
        access_log.open("../logs/access.log", std::ios::out | std::ios::app);
        error_log.open("../logs/error.log", std::ios::out | std::ios::app);

        if (!access_log.is_open() || !error_log.is_open()) {
            std::cerr << "Failed to open log files\n";
        }
    }

    void log_access(const std::string& client_ip,
                   const std::string& method,
                   const std::string& path,
                   int status_code,
                   long long response_time_ms) {
        std::lock_guard lock(log_mutex);
        if (access_log.is_open()) {
            access_log << "[" << get_timestamp() << "] "
                      << client_ip << " "
                      << method << " "
                      << path << " "
                      << status_code << " "
                      << response_time_ms << "ms\n";
            // access_log.flush();
        }
    }

    void log_error(const std::string& message) {
        std::lock_guard lock(log_mutex);
        if (error_log.is_open()) {
            error_log << "[" << get_timestamp() << "] ERROR: "
                     << message << "\n";
            // error_log.flush();
        }
    }

    ~Logger() {
        if (access_log.is_open()) {
            access_log.close();
        }
        if (error_log.is_open()) {
            error_log.close();
        }
    }
};

#endif //HTTP_SERVER_LOGGER_H
