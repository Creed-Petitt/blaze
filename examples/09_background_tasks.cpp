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

// A background worker that checks GitHub's status every 10 seconds
Async<void> github_monitor(App& app) {
    while (true) {
        try {
            std::cout << "[Monitor] Checking GitHub status..." << std::endl;
            auto res = co_await blaze::fetch("https://api.github.com/zen");
            
            if (res.status == 200) {
                std::cout << "[Monitor] GitHub is UP. Zen: " << res.text() << std::endl;
            } else {
                std::cout << "[Monitor] GitHub returned status: " << res.status << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[Monitor] GitHub check failed: " << e.what() << std::endl;
        }

        // Wait for 10 seconds WITHOUT blocking the worker thread
        co_await blaze::delay(std::chrono::seconds(10));
    }
}

int main() {
    App app;

    // 1. Launch the background worker
    // The server will handle requests while this runs in parallel
    app.spawn(github_monitor(app));

    app.get("/", [](Response& res) -> Async<void> {
        res.send("The background monitor is running. Check your terminal!");
        co_return;
    });

    std::cout << "Background task demo running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
