/**
 * Example 09: Background Tasks
 * 
 * This example demonstrates how to run asynchronous tasks in the background.
 * Concepts:
 * - app.spawn() for background coroutines
 * - blaze::delay() for non-blocking timers
 * - Periodic background work
 */

#include <blaze/app.h>
#include <blaze/client.h>

using namespace blaze;

// A background worker that checks an HTTP endpoint every 10 seconds
Async<void> status_monitor(App& app) {
    while (true) {
        try {
            std::cout << "[Monitor] Checking example.com..." << std::endl;
            auto res = co_await blaze::fetch("http://example.com/");
            
            if (res.status == 200) {
                std::cout << "[Monitor] example.com is reachable." << std::endl;
            } else {
                std::cout << "[Monitor] example.com returned status: " << res.status << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[Monitor] HTTP check failed: " << e.what() << std::endl;
        }

        // Wait for 10 seconds WITHOUT blocking the worker thread
        co_await blaze::delay(std::chrono::seconds(10));
    }
}

int main() {
    App app;

    // 1. Launch the background worker
    // The server will handle requests while this runs in parallel
    app.spawn(status_monitor(app));

    app.get("/", [](Response& res) -> Async<void> {
        res.send("The background monitor is running. Check your terminal!");
        co_return;
    });

    std::cout << "Background task demo running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
