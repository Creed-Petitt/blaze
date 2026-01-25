# Middleware & Security

Blaze uses a simple middleware chain (Request -> MW1 -> MW2 -> Handler -> MW2 -> MW1 -> Response).

## 1. Global Middleware
Register middleware that runs for every single request using `app.use()`.

```cpp
app.use(middleware::logger());
app.use(middleware::cors()); // Enable Cross-Origin Resource Sharing
app.use(middleware::static_files("public"));
```

## 2. Authentication (JWT)
Blaze includes built-in JWT authentication.

### Setup
```cpp
auto auth = middleware::jwt_auth("your_secret_key");
// By default, this is "Optional Auth". It checks tokens if present, but allows guests.
app.use(auth);
```

### Accessing the User in Routes
Use the `req.user()` helper to access the parsed JWT payload safely.

```cpp
app.get("/profile", [](Request& req, Response& res) -> Async<void> {
    if (!req.is_authenticated()) {
        throw Unauthorized("Please log in");
    }

    // Direct access to user JSON (Pointer-free)
    int id = req.user()["id"].as<int>();
    res.json(req.user());
});
```

## 3. Rate Limiting
Protect your API from abuse with the built-in rate limiter.

```cpp
// Allow 100 requests per 60 seconds per IP
app.use(middleware::rate_limit(100, 60));
```

## 4. Context Storage
Pass data between middleware and handlers using the typed context bucket.

```cpp
// Middleware
req.set("request_id", std::string("req-123"));

// Handler (using Context wrapper)
app.get("/", [](Context<std::string> rid) { ... });
```

## 5. Static Files
Blaze can serve static assets (HTML, CSS, JS) with automatic mime-type detection.

```cpp
// Serve the 'public' folder at the root
app.use(middleware::static_files("public"));
```

## 4. Custom Middleware
You can write your own middleware using the standard Blaze signature.

```cpp
app.use([](Request& req, Response& res, Next next) -> Task {
    auto start = std::chrono::steady_clock::now();
    
    co_await next(); // Pass control to the next middleware or handler
    
    auto end = std::chrono::steady_clock::now();
    // Logic here runs AFTER the handler finished
    std::cout << "Request took " << duration(start, end) << "ms" << std::endl;
});
```
