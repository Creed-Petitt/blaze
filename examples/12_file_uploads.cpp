/**
 * Example 12: Advanced File Uploads
 * 
 * This example demonstrates the high-level Multipart API for handling
 * file uploads and form data.
 * 
 * Concepts:
 * - Server-side: Using req.form() and req.files() (Zero-Copy)
 * - Client-side: Using fetch(url, MultipartFormData)
 */

#include <blaze/app.h>
#include <blaze/client.h>
#include <iostream>
#include <filesystem>

using namespace blaze;
namespace fs = std::filesystem;

int main() {
    App app;
    fs::create_directories("./uploads");

    // SERVER

    app.get("/", [](Response& res) -> Async<void> {
        res.header("Content-Type", "text/html").send(R"(
            <form action="/upload" method="post" enctype="multipart/form-data">
                <input type="text" name="user" placeholder="Your Name"><br>
                <input type="file" name="photo"><br>
                <input type="submit" value="Upload">
            </form>
        )");
        co_return;
    });

    app.post("/upload", [](Request& req, Response& res) -> Async<void> {
        const auto& form = req.form();
        
        auto user = form.get_field("user").value_or("Anonymous");
        const auto* photo = form.get_file("photo");

        if (photo) {
            std::string path = "./uploads/" + photo->filename;
            photo->save_to(path);
            std::cout << "[Server] Saved " << photo->filename << " for user " << user << std::endl;
            res.send("Successfully uploaded " + photo->filename);
        } else {
            res.status(400).send("No photo uploaded");
        }
        co_return;
    });

    // CLIENT

    // We spawn a client task that uploads a virtual file to this server
    app.spawn([](App& a) -> Async<void> {
        // Wait for server to start
        co_await blaze::delay(std::chrono::milliseconds(500));

        MultipartFormData form;
        form.add_field("user", "BlazeClient");
        form.add_file("photo", "test_upload.txt", "This is a test file content", "text/plain");

        std::cout << "[Client] Starting multipart upload..." << std::endl;
        try {
            auto res = co_await fetch("http://localhost:8080/upload", form);
            std::cout << "[Client] Server responded: " << res.text() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Client] Upload failed: " << e.what() << std::endl;
        }
    }(app));

    std::cout << "Upload Server running on http://localhost:8080" << std::endl;
    app.listen(8080);

    return 0;
}
