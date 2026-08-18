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
#include <iostream>

using namespace blaze;

// A background worker that runs every 10 seconds without blocking the server.
Async<void> status_monitor() {
    int tick = 0;
    while (true) {
        std::cout << "[Monitor] background tick " << ++tick << std::endl;
        co_await blaze::delay(std::chrono::seconds(10));
    }
}

int main() {
    App app;

    // 1. Launch the background worker
    // The server will handle requests while this runs in parallel
    app.spawn(status_monitor());

    app.get("/", [](Response& res) -> Async<void> {
        res.send("The background monitor is running. Check your terminal!");
        co_return;
    });

    std::cout << "Background task demo running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
