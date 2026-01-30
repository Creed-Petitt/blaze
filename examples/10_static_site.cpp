/**
 * Example 10: Static Site
 * 
 * This example demonstrates how to serve a static website with Blaze.
 * Concepts:
 * - middleware::static_files()
 * - RAM Caching for high performance
 * - Serving index.html automatically
 */

#include <blaze/app.h>
#include <blaze/middleware.h>

using namespace blaze;

int main() {
    // Create the folder and a sample file automatically for this demo
    // We do this BEFORE registering middleware so canonical path resolution works
    std::filesystem::create_directories("./public");
    std::ofstream out("./public/index.html");
    out << "<html><body><h1>Welcome to Blaze Static Site</h1><p>Served from RAM cache!</p></body></html>";
    out.close();

    App app;

    // 1. Register the static files middleware
    // This will serve files from the './public' directory.
    // Blaze will automatically cache files in RAM after the first read.
    app.use(middleware::static_files("./public"));

    // 2. Add an API route alongside the static site
    app.get("/api/status", [](Response& res) -> Async<void> {
        res.json({{"status", "api_online"}});
        co_return;
    });

    // Instructions for the user
    std::cout << "Static Site demo running on :8080" << std::endl;
    std::cout << "Create a './public' folder and add an 'index.html' to see it in action!" << std::endl;

    app.listen(8080);

    return 0;
}
