# Advanced Features & Utilities

This guide covers advanced organizational tools and core utilities in the Blaze framework.

## 1. API Route Groups
For large applications, you can group related routes under a common prefix. This is ideal for versioning (e.g., `/api/v1`) or resource grouping.

```cpp
// Create a group for user-related endpoints
auto users = app.group("/users");

// These routes will be /users/profile and /users/settings
users.get("/profile", [](Response& res) { ... });
users.get("/settings", [](Response& res) { ... });

// You can even nest groups
auto v1 = app.group("/api/v1");
auto admin = v1.group("/admin"); // Result: /api/v1/admin
```

## 2. Environment Variables (.env)
Blaze includes a built-in environment manager that automatically loads `.env` files and provides type-safe access to variables.

### Loading the File
Usually done at the top of `main()`:
```cpp
blaze::load_env(); // Defaults to ".env"
```

### Accessing Variables (Type-Safe)
The `blaze::env<T>` template allows you to fetch and cast variables in one line.

```cpp
// Get a string (throws if missing)
std::string key = env("API_KEY");

// Get with a default value (no throw)
int port = env<int>("PORT", 8080);

// Get a boolean
bool debug = env<bool>("DEBUG", false);
```

## 3. Customizing the Server
You can fine-tune the engine's performance through the `AppConfig`.

```cpp
App app;

// Set max body size (e.g., for file uploads)
app.max_body_size(50 * 1024 * 1024); // 50MB

// Access config directly
app.config().timeout_seconds = 60;

// Redirect framework logs to a file
app.log_to("server.log");
```

## 4. The Response Builder (Manual Mode)
While `Async<T>` is recommended, you can use the fluent `Response` API for full control.

```cpp
app.get("/custom", [](Response& res) -> Async<void> {
    res.status(201)
       .header("X-Custom-Header", "Blaze")
       .json({{"status", "created"}});
    co_return;
});
```

## 5. Built-in HTTP Client (blaze::fetch)
Blaze includes a non-blocking HTTP client that uses the same coroutine engine as the server. This is perfect for calling external APIs or microservices.

### Basic Usage
```cpp
#include <blaze/client.h>

app.get("/proxy", []() -> Async<Json> {
    // Non-blocking GET request
    auto response = co_await blaze::fetch("https://api.github.com/repos/Creed-Petitt/blaze");
    
    if (response.status == 200) {
        co_return response.body; // response.body is a blaze::Json object
    }
    
    throw InternalServerError("Upstream API failed");
});
```

### Advanced POST Request
```cpp
auto res = co_await blaze::fetch("https://api.example.com/v1/data", "POST", 
    {{"Authorization", "Bearer secret"}}, 
    Json({{"key", "value"}})
);
```

## 6. Automatic API Docs (Swagger)
Blaze automatically generates an OpenAPI 3.0 specification for your API using compile-time reflection.

### Accessing the Docs
When you run your application, the following routes are registered automatically:
*   `GET /openapi.json`: The raw OpenAPI specification.
*   `GET /docs`: An interactive Swagger UI.

### How it works
The documentation engine inspects your route handlers and models. If you use `Body<User>`, the engine automatically generates a JSON Schema for the `User` struct and includes it in the spec. No annotations or manual configuration required.
