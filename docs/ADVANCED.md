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
app.get("/custom", [](Response& res) -> Task {
    res.status(201)
       .header("X-Custom-Header", "Blaze")
       .json({{"status", "created"}});
    co_return;
});
```
