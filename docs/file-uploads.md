# File Uploads & Multipart Data

Blaze provides built-in support for parsing `multipart/form-data` requests. This allows you to easily handle file uploads and complex forms in your API.

---

## 1. Accessing Form Data

You can access multipart data directly from the `Request` object using `req.form()`. This returns a `MultipartFormData` object that contains all fields and files.

```cpp
app.post("/upload", [](Request& req) -> Async<Json> {
    const auto& form = req.form();
    
    // Get a regular form field
    auto desc = form.get_field("description").value_or("No description");
    
    // Get a file
    const auto* file = form.get_file("avatar");
    if (file) {
        std::cout << "Saving " << file->filename << std::endl;
        file->save_to("./uploads/" + file->filename);
    }

    co_return Json({{"status", "success"}});
});
```

---

## 2. Handling Multiple Files

If your form supports multiple file uploads, you can iterate through them using `form.files()`.

```cpp
app.post("/gallery", [](Request& req) -> Async<void> {
    auto files = req.files(); // Helper for form().files()
    
    for (const auto* file : files) {
        file->save_to("./gallery/" + file->filename);
    }
    
    res.send("Uploaded " + std::to_string(files.size()) + " files");
    co_return;
});
```

---

## 3. The `MultipartPart` Object

Each part in the form is represented by a `MultipartPart` object:

*   **`.name`**: The name of the form field (e.g., `"username"`).
*   **`.filename`**: The original filename from the client (e.g., `"profile.jpg"`). Empty for regular fields.
*   **`.content_type`**: The MIME type of the file (e.g., `"image/jpeg"`).
*   **`.data`**: A `std::string_view` pointing to the raw bytes of the file in memory.
*   **`.save_to(path)`**: A helper method to write the data to the specified disk path.

---

## 4. Performance & Limits

Blaze uses an **In-Memory** parser. This means the entire file is held in RAM during the request. 

### **Best Practices:**
1.  **Memory Usage**: For very large files (e.g., 1GB video), ensure your server has enough RAM.
2.  **Request Limits**: You can configure the maximum allowed body size in the app config to prevent memory exhaustion:
    ```cpp
    app.max_body_size(50 * 1024 * 1024); // Limit to 50MB
    ```
3.  **Zero-Copy**: The parser uses `std::string_view`, meaning it doesn't copy the file data during parsing—it points directly to the already-downloaded request body.

---

## 5. Client-Side Uploads

You can also use the `MultipartFormData` class to **send** files using the Blaze Async Client.

```cpp
#include <blaze/client.h>
#include <blaze/multipart.h>

// Create the form
MultipartFormData form;
form.add_field("user_id", "101");
form.add_file("document", "report.pdf", file_content, "application/pdf");

// Send via POST
auto res = co_await blaze::fetch("http://example.com/upload", form);
```

For more details, see the [HTTP Client Documentation](http-client.md).
