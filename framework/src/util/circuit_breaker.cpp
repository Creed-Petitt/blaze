#include <blaze/util/circuit_breaker.h>
#include <chrono>

namespace blaze {

CircuitBreaker::CircuitBreaker(int threshold, int cooldown_seconds)
    : threshold_(threshold), 
      cooldown_ns_(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(cooldown_seconds)).count()) {}

bool CircuitBreaker::allow_request() {
    if (fail_count_.load(std::memory_order_acquire) < threshold_) return true;
    
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto last = last_fail_time_ns_.load(std::memory_order_acquire);
    
    if (now - last > cooldown_ns_) {
        // Half-open: Allow one request to probe the system
        return true;
    }
    return false;
}

void CircuitBreaker::record_success() {
    fail_count_.store(0, std::memory_order_release);
}

void CircuitBreaker::record_failure() {
    fail_count_.fetch_add(1, std::memory_order_acq_rel);
    last_fail_time_ns_.store(
        std::chrono::steady_clock::now().time_since_epoch().count(), 
        std::memory_order_release
    );
}

} // namespace blaze
