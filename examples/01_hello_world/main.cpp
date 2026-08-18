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
#include <iostream>

#include "../../include/blaze/config.h"

using namespace blaze;

int main() {

    // Create and configure the application
    auto cfg = ConfigBuilder()
        .server_name("HelloWorldApp/1.0")
        .log_level(LogLevel::DEBUG)
        .threads(4)
        .log_path("/dev/null")
        .build();

    App app{cfg};

    // Define a simple route
    // Handlers are lambdas that return Async<void> (a C++20 coroutine)
    app.get("/", [](Response& res) -> Async<void> {
        // Send a plain text response
        res.send("Hello from Blaze!");
        co_return;
    });

    // Start the server on port 8080
    // This call is blocking and starts the internal thread pool
    std::cout << "Starting server on http://localhost:8080" << std::endl;
    app.listen(8080);

    return 0;
}
