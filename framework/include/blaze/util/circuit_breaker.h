#ifndef BLAZE_UTIL_CIRCUIT_BREAKER_H
#define BLAZE_UTIL_CIRCUIT_BREAKER_H

#include <atomic>
#include <chrono>

namespace blaze {

/**
 * @brief Generic Circuit Breaker to prevent cascading failures.
 * Uses Acquire/Release semantics for high-concurrency safety.
 */
class CircuitBreaker {
public:
    CircuitBreaker(int threshold = 5, int cooldown_seconds = 5);

    /**
     * @brief Checks if a request should be allowed.
     * @return true if allowed, false if "Open".
     */
    bool allow_request();

    /**
     * @brief Resets the failure counter on success.
     */
    void record_success();

    /**
     * @brief Increments failure counter and trips the breaker if threshold is reached.
     */
    void record_failure();

private:
    std::atomic<int> fail_count_{0};
    std::atomic<int64_t> last_fail_time_ns_{0};
    int threshold_;
    int64_t cooldown_ns_;
};

} // namespace blaze

#endif // BLAZE_UTIL_CIRCUIT_BREAKER_H
