/**
 * Example 06: Middleware Chain
 * 
 * This example demonstrates how to build custom middleware and use the Request Context.
 * Concepts:
 * - Custom middleware with the Next function
 * - Request-scoped context (Context<T>)
 * - Logging and timing
 */

#include <blaze/app.h>
#include <chrono>

using namespace blaze;

int main() {
    App app;

    // 1. Custom Timer Middleware
    app.use([](Request& req, Response& res, Next next) -> Async<void> {
        auto start = std::chrono::steady_clock::now();

        // Pass control to the next middleware or handler
        co_await next();

        auto end = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        // Add a custom header to the response
        res.header("X-Response-Time-US", std::to_string(diff));
        co_return;
    });

    // 2. Custom Trace ID Middleware
    app.use([](Request& req, Response& res, Next next) -> Async<void> {
        // Generate a simple trace ID
        std::string trace_id = "trace-" + std::to_string(std::time(nullptr));
        
        // Store it in the request context
        req.set("trace_id", trace_id);

        co_await next();
        co_return;
    });

    // 3. Handler using the Context
    app.get("/", [](Context<std::string> trace_id) -> Async<Json> {
        // Context<T> automatically retrieves the value from the request context
        co_return Json({
            {"message", "Middleware trace demo"},
            {"your_trace_id", trace_id}
        });
    });

    std::cout << "Middleware demo running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
