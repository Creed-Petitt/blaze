/**
 * Example 08: Production SSL
 * 
 * This example demonstrates how to run a secure production-ready server.
 * Concepts:
 * - HTTPS / SSL configuration
 * - Production logging
 * - Server tuning (max body size)
 */

#include <blaze/app.h>

using namespace blaze;

int main() {
    App app;

    // 1. Configure for production
    app.server_name("Blaze-Secure/1.1")
       .log_to("production.log")
       .log_level(LogLevel::INFO)
       .max_body_size(50 * 1024 * 1024)
       .timeout(30)
       .num_threads(8);

    app.get("/", [](Response& res) -> Async<void> {
        res.send("This is a secure HTTPS connection!");
        co_return;
    });

    // 2. Start the HTTPS server
    // Note: You need valid .pem files for this to work.
    // For testing, you can generate self-signed ones:
    // openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
    try {
        std::cout << "Starting HTTPS server on port 443..." << std::endl;
        // app.listen_ssl(443, "cert.pem", "key.pem");
        
        // Falling back to 8443 for local testing without root
        app.listen_ssl(8443, "cert.pem", "key.pem");
    } catch (const std::exception& e) {
        std::cerr << "Failed to start SSL server: " << e.what() << std::endl;
        std::cerr << "Make sure cert.pem and key.pem exist!" << std::endl;
    }

    return 0;
}
