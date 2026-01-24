# WebSockets & Real-time Communication

Blaze includes a built-in WebSocket registry that handles session management, thread-safety, and broadcasting automatically.

## 1. Defining WebSocket Routes
Register a WebSocket path using `app.ws()`. Sessions are automatically tracked by the framework upon a successful handshake.

```cpp
app.ws("/feed", {
    .on_open = [](std::shared_ptr<WebSocket> ws) {
        std::cout << "Client connected to feed" << std::endl;
    },
    .on_message = [](auto ws, std::string msg) {
        ws->send("Echo: " + msg);
    },
    .on_close = [](auto ws) {
        std::cout << "Client disconnected" << std::endl;
    }
});
```

## 2. Global Broadcasting
Blaze maintains a path-based registry. You can broadcast a message to every client connected to a specific path with a single call.

### Efficiency
Serialization is performed exactly once per broadcast ($O(1)$ serialization overhead), regardless of the number of connected clients.

```cpp
struct Update {
    std::string type;
    double value;
};
BLAZE_MODEL(Update, type, value)

// ... inside a loop or handler ...
Update data = {"ticker", 89500.50};
app.broadcast("/feed", data); // Sent to all clients on /feed
```

## 3. Background Tasks
For continuous tasks like price monitoring or heartbeats, use `app.spawn()`. This launches a coroutine into the server's main event loop.

```cpp
// 1. Define your background logic
Task monitor_loop(App& app) {
    while (true) {
        app.broadcast("/heartbeat", Json({{"status", "alive"}}));
        
        // Wait without blocking a thread
        co_await blaze::delay(std::chrono::seconds(1));
    }
}

// 2. Spawn it during startup
app.spawn(monitor_loop(app));
```

## 4. Async Utilities

### blaze::delay
A non-blocking alternative to `std::this_thread::sleep_for`. It suspends the current coroutine, freeing the thread to handle other requests.

```cpp
co_await blaze::delay(std::chrono::milliseconds(500));
```

## 5. Automatic Cleanup
The WebSocket registry uses `std::weak_ptr` to track sessions. If a client disconnects or their connection drops, Blaze automatically prunes them from the registry during the next broadcast, preventing memory leaks and stale transmissions.
