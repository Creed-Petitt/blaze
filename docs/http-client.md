# Async HTTP Client

Blaze includes a high-performance, non-blocking HTTP client (`blaze::fetch`). It is built on the same `boost::beast` and C++20 coroutine engine as the server, ensuring efficient I/O usage.

## 1. Basic Usage

The `fetch` function is a coroutine that returns a `FetchResponse`.

```cpp
#include <blaze/client.h>

app.get("/proxy", []() -> Async<void> {
    // Simple GET request
    auto res = co_await blaze::fetch("https://api.github.com/zen");
    
    if (res.status == 200) {
        std::cout << "GitHub Zen: " << res.text() << std::endl;
    }
    co_return;
});
```

## 2. JSON Requests (POST/PUT)

If you pass a `blaze::Json` object as the body, the client automatically sets the `Content-Type: application/json` header and serializes the data.

```cpp
blaze::Json payload = {
    {"username", "creed"},
    {"role", "admin"}
};

// POST request with JSON body
auto res = co_await blaze::fetch(
    "https://api.example.com/users", 
    "POST", 
    {{"Authorization", "Bearer token123"}}, // Headers map
    payload
);
```

## 3. Advanced Features

### Timeouts
You can specify a timeout in seconds. If the request takes longer, a `boost::system::system_error` (or `std::runtime_error`) is thrown.

```cpp
// Timeout after 5 seconds (Default is 30s)
try {
    auto res = co_await blaze::fetch("https://slow-api.com", "GET", {}, {}, 5);
} catch (const std::exception& e) {
    std::cerr << "Request timed out: " << e.what() << std::endl;
}
```

### Automatic Redirects
The client automatically follows up to **10 redirects** (HTTP 301, 302, 303, 307, 308).
*   For 301, 302, and 303, it converts the method to `GET` and drops the body (standard browser behavior).
*   For 307 and 308, it preserves the method and body.

### Multipart Uploads
You can upload files and forms using the `MultipartFormData` helper. This is the same class used for parsing server-side requests.

```cpp
#include <blaze/multipart.h>

MultipartFormData form;
form.add_field("description", "File upload test");
form.add_file("avatar", "me.png", raw_file_bytes, "image/png");

// Automatically sets 'Content-Type: multipart/form-data; boundary=...'
auto res = co_await blaze::fetch("https://api.example.com/upload", form);
```

## 4. The FetchResponse Object

The response object contains:

*   **`status`** (`int`): The HTTP status code (e.g., 200, 404).
*   **`headers`** (`std::map<std::string, std::string>`): Case-sensitive headers.
    *   Helper: `get_header("content-type")` (Case-insensitive lookup).
    *   Helper: `get_headers("set-cookie")` (Returns `vector<string>` for multi-value headers).
*   **`body`** (`Json`): The parsed JSON body (if applicable).
*   **`text()`**: Helper method to get the body as a raw string.
