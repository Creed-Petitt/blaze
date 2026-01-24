# Middleware & Security

Blaze uses a powerful middleware chain based on the "Onion" model (Request -> MW1 -> MW2 -> Handler -> MW2 -> MW1 -> Response).

## 1. Global Middleware
Register middleware that runs for every single request using `app.use()`.

```cpp
app.use(middleware::logger());
app.use(middleware::static_files("public"));
```

## 2. Authentication (JWT)
Blaze includes built-in JWT authentication that integrates directly with the Request object.

### Setup
```cpp
auto auth = middleware::jwt_auth("your_secret_key");
app.use(auth);
```

### Accessing the User in Routes
Once the middleware is active, you can check if a user is authenticated directly from the request.

```cpp
app.get("/profile", [](Request& req, Response& res) -> Task {
    auto user = req.user(); // Returns optional<Json>
    
    if (!user) {
        throw Unauthorized("Please log in");
    }

    res.json({
        {"message", "Welcome back!"},
        {"user_id", (*user)["id"]}
    });
});
```

## 3. Static Files
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
