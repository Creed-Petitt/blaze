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
Blaze features a built-in identity system for tracking authenticated users. This is a "Producer/Consumer" model:

1.  **The Producer**: Middleware (like `jwt_auth`) validates credentials and attaches an identity to the request via `req.set_user(json_payload)`.
2.  **The Consumer**: Your route handlers check if a user is present and access their data.

```cpp
app.get("/profile", [](Request& req) -> Async<Json> {
    // 1. Check if the user is logged in
    if (!req.is_authenticated()) {
        throw Unauthorized("You must be logged in to view this page");
    }

    // 2. Access the user data (from the JWT payload)
    Json user = req.user();
    std::cout << "Viewing profile for: " << user["email"] << std::endl;

    co_return user;
});
```

---

## 4. Built-in Security

### JWT Authentication
Blaze includes a built-in JWT validator that uses **constant-time (timing-safe)** string comparisons to prevent side-channel attacks. It automatically checks the `Authorization: Bearer <token>` header.

**Behavior:**
*   **Success**: If the token is valid, it extracts the payload, calls `req.set_user()`, and sets `req.is_authenticated()` to **true**.
*   **Missing/Invalid**: It allows the request to continue (so you can have optional auth), but `req.is_authenticated()` will be **false**.

```cpp
// Protect your routes with a secret key
app.use(middleware::jwt_auth("your-secret-key"));
```

### Rate Limiting
Protect your API from abuse by limiting the number of requests per IP address.

```cpp
// Allow 100 requests per 60 seconds per IP
app.use(middleware::rate_limit(100, 60));
```

### Static File Caching
The `static_files` middleware includes a thread-safe cache. This means that after the first time a file (like `logo.png`) is read from the disk, it is served directly from RAM for all future users, drastically increasing performance.

---

## 5. Crypto & Password Utilities

Security isn't just about middleware; it's about how you handle data. Blaze includes a `blaze::crypto` namespace for common security tasks.

### Password Hashing
Never store passwords in plain text. Blaze provides a built-in hashing utility that uses **SCRYPT**, a memory-hard algorithm designed to be resistant to hardware (ASIC/GPU) brute-force attacks.

```cpp
#include <blaze/crypto.h>

// 1. Hash a password during signup
// Automatically handles salting and work-factor tuning
std::string hash = blaze::crypto::hash_password("user_password");

// 2. Verify during login (Timing-safe)
bool ok = blaze::crypto::verify_password("user_password", hash);
```

### Manual JWT Management
While `jwt_auth` middleware handles verification, you need to sign tokens yourself when a user logs in.

```cpp
// Generate a token that expires in 1 hour (3600s)
Json payload = {{"user_id", 123}, {"role", "admin"}};
std::string token = blaze::crypto::jwt_sign(payload, "your-secret-key", 3600);

// Return it to the client
res.json({{"token", token}});
```

### Core Cryptography
*   **`sha256(data)`**: Returns a hex-encoded SHA256 hash.
*   **`random_token(length)`**: Generates a secure random string (perfect for API keys or session IDs).
*   **`base64_encode/decode`**: Standard base64 and base64url support.
