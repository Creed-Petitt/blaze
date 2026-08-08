# Middleware & Security

Middleware is a powerful way to extend Blaze. It allows you to run code for every request, either before it reaches your route handler or after it finishes.

---

## 1. The "Onion" Model

Think of your application like an onion. Each middleware is a layer that "wraps" the inner layers and the core (your route handler).

1.  **Incoming Request**: Passes through Middleware A -> Middleware B -> Your Handler.
2.  **Outgoing Response**: Passes back through Your Handler -> Middleware B -> Middleware A.

```cpp
app.use([](Request& req, Response& res, Next next) -> Async<void> {
    // 1. Code here runs BEFORE the handler
    auto start = std::chrono::steady_clock::now();

    co_await next(); // 2. Pass control to the next layer

    // 3. Code here runs AFTER the handler finishes
    auto end = std::chrono::steady_clock::now();
    std::cout << "Request took: " << duration(start, end) << "ms" << std::endl;
    co_return;
});
```

---

## 2. Global Middleware

Register middleware that applies to your entire application using `app.use()`.

```cpp
app.use(middleware::logger());      // Logs every request
app.use(middleware::cors());        // Enables Cross-Origin Resource Sharing
app.use(middleware::static_files("public")); // Serves assets from the 'public' folder
```

---

## 3. Passing Data (`Request Context`)

Middleware often generates data that your routes need (like a User ID or a Trace ID). Blaze provides two ways to pass this data:

### Type-Safe Context (`Context<T>`)
Blaze uses a **Type-Based** injection system. To use injection, you must store data using the C++ type as the key.

```cpp
// 1. Define a unique struct for your data
struct RequestTrace {
    std::string id;
};

// 2. In Middleware (Set by Type)
app.use([](Request& req, Response& res, Next next) -> Async<void> {
    // This stores the object using RequestTrace as the key
    req.set(RequestTrace{"uuid-123"});
    co_await next();
});

// 3. In Route Handler (Magic Injection)
app.get("/", [](Context<RequestTrace> trace) {
    // 'trace' acts like RequestTrace*
    std::cout << "Trace: " << trace->id << std::endl;
});
```

### Key-Value Context
If you prefer manual control or simple string keys, you can set and get values manually. Note that these **cannot** be injected via `Context<T>`.

```cpp
// Set
req.set("session_id", "abc-123");

// Get
std::string session = req.get<std::string>("session_id");
```

### User Identity & `is_authenticated()`
Blaze includes request-level identity storage for applications that provide their own authentication middleware. This is a "Producer/Consumer" model:

1.  **The Producer**: Your middleware validates credentials and attaches an identity to the request via `req.set_user(json_payload)`.
2.  **The Consumer**: Your route handlers check if a user is present and access their data.

```cpp
app.get("/profile", [](Request& req) -> Async<Json> {
    // 1. Check if the user is logged in
    if (!req.is_authenticated()) {
        throw Unauthorized("You must be logged in to view this page");
    }

    // 2. Access the user data
    Json user = req.user();
    std::cout << "Viewing profile for: " << user["email"] << std::endl;

    co_return user;
});
```

---

## 4. Built-in Middleware

### Rate Limiting
Protect your API from abuse by limiting the number of requests per IP address.

```cpp
// Allow 100 requests per 60 seconds per IP
app.use(middleware::rate_limit(100, 60));
```

### Zero-Copy File Streaming
The `static_files` middleware uses high-performance **Zero-Copy Streaming**. Unlike traditional frameworks that read a file into a memory buffer before sending it, Blaze uses `boost::beast::http::file_body`.

**Benefits:**
*   **Zero RAM Overhead**: Whether you serve a 1KB icon or a 10GB video, Blaze consumes virtually no additional memory.
*   **Kernel-Level Efficiency**: The OS handles the data transfer directly from the disk cache to the network socket.
*   **Automatic MIME Detection**: Blaze automatically detects and caches MIME types for all common web formats.

```cpp
app.use(middleware::static_files("public"));
```
