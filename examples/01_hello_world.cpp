/**
 * Example 01: Hello World
 * 
 * This example demonstrates the most basic Blaze application.
 * Concepts:
 * - App instance creation
 * - Basic GET routing
 * - Using the Response object to send text
 * - Starting the server
 */

#include <blaze/app.h>

using namespace blaze;

int main() {
    // 1. Create and configure the application
    App app;
    
    app.server_name("HelloWorldApp/1.0")
       .log_level(LogLevel::DEBUG);

    // 2. Define a simple route
    // Handlers are lambdas that return Async<void> (a C++20 coroutine)
    app.get("/", [](Response& res) -> Async<void> {
        // Send a plain text response
        res.send("Hello from Blaze!");
        co_return;
    });

    // 3. Start the server on port 8080
    // This call is blocking and starts the internal thread pool
    std::cout << "Starting server on http://localhost:8080" << std::endl;
    app.listen(8080);

    return 0;
}
