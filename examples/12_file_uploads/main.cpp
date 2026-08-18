/**
 * Example 12: Advanced File Uploads
 * 
 * This example demonstrates the high-level Multipart API for handling
 * file uploads and form data.
 * 
 * Concepts:
 * - Server-side: Using req.form() and req.files() (Zero-Copy)
 */

#include <blaze/app.h>
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

    std::cout << "Upload Server running on http://localhost:8080" << std::endl;
    app.listen(8080);

    return 0;
}
