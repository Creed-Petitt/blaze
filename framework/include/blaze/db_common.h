#ifndef BLAZE_DB_COMMON_H
#define BLAZE_DB_COMMON_H

#include <atomic>
#include <chrono>

namespace blaze {

class CircuitBreaker {
public:
    CircuitBreaker(int threshold = 5, int cooldown_seconds = 5)
        : threshold_(threshold), cooldown_ns_(cooldown_seconds * 1'000'000'000LL) {}

    bool allow_request() {
        if (fail_count_.load(std::memory_order_relaxed) < threshold_) return true;
        
        auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        if (now - last_fail_time_.load(std::memory_order_relaxed) > cooldown_ns_) {
            // Half-open: Allow one request
            return true;
        }
        return false;
    }

    void record_success() {
        fail_count_.store(0, std::memory_order_relaxed);
    }

    void record_failure() {
        fail_count_.fetch_add(1, std::memory_order_relaxed);
        last_fail_time_.store(
            std::chrono::steady_clock::now().time_since_epoch().count(), 
            std::memory_order_relaxed
        );
    }

private:
    std::atomic<int> fail_count_{0};
    std::atomic<long long> last_fail_time_{0};
    int threshold_;
    long long cooldown_ns_;
};

} // namespace blaze

#endif
