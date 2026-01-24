# Blaze v1.1 Documentation

Welcome to the official manual for the Blaze Framework.

## Detailed Guides

To get the most out of Blaze, explore our detailed guides:

1.  **[The CLI](CLI.md)**: Master the build system, `blaze add`, and `blaze doctor`.
2.  **[Dependency Injection](DI.md)**: Learn how to use the IoC container and Magic Route Injection.
3.  **[Database & ORM](DATABASE.md)**: Setting up drivers and using the `BLAZE_MODEL` macro.
4.  **[WebSockets & Real-time](WEBSOCKETS.md)**: Scaling to thousands of connections with the Dream API.
5.  **[Middleware & Security](MIDDLEWARE.md)**: JWT Authentication, Logging, and Static Files.
6.  **[Advanced Features](ADVANCED.md)**: Route Groups, Environment variables, and Server configuration.

---

## Quick Start (60 Seconds)

### 1. Create a Project
```bash
blaze init my-app
cd my-app
```

### 2. Define a Route
In `src/main.cpp`:
```cpp
app.get("/", []() -> Async<Json> {
    co_return Json({{"message", "Hello Blaze!"}});
});
```

### 3. Run it
```bash
blaze run
```

---

## Framework Philosophy
Blaze is built on three core pillars:
1.  **Zero Manual Memory Management**: Use `shared_ptr` and `co_await`.
2.  **Modular by Design**: Only link what you use.
3.  **Modern C++20**: Leverage coroutines for simple, high-performance async logic.