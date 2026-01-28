# WebSockets & Real-time

Blaze provides a first-class WebSocket implementation that is fully integrated into its asynchronous event loop. It is designed to handle thousands of concurrent real-time connections with minimal overhead.

---

## 1. Defining WebSocket Routes

You register a WebSocket endpoint using `app.ws()`. Instead of a single handler, you provide a set of lifecycle callbacks.

```cpp
app.ws("/chat", {
    .on_open = [](std::shared_ptr<WebSocket> ws) {
        std::cout << "A user joined the chat!" << std::endl;
        ws->send("Welcome!");
    },
    .on_message = [](auto ws, std::string msg) {
        std::cout << "Received: " << msg << std::endl;
        ws->send("Echo: " + msg);
    },
    .on_close = [](auto ws) {
        std::cout << "User left." << std::endl;
    }
});
```

### Lifecycle Events:
*   **`on_open`**: Triggered when a client successfully completes the WebSocket handshake.
*   **`on_message`**: Triggered whenever the client sends a text message.
*   **`on_close`**: Triggered when the connection is terminated (either by the client or the server).

---

## 2. Global Broadcasting

One of Blaze's standout features is its **Path-Based Registry**. Blaze automatically tracks every client connected to a specific path (e.g., `/chat`).

### Why is it efficient?
When you broadcast a message to 10,000 users, most frameworks serialize the data 10,000 times. **Blaze serializes the data exactly ONCE**, and then pushes the raw bytes to all connected sockets. This is an $O(1)$ serialization operation.

```cpp
struct Update {
    std::string type;
    double value;
};
BLAZE_MODEL(Update, type, value)

// Anywhere in your app, you can broadcast to all clients on "/market"
app.broadcast("/market", Update{"BTC", 95000.0});
```

---

## 3. Background Tasks (`app.spawn`)

Real-time apps often need background loops (e.g., a "Ghost" process that checks prices every second). Use `app.spawn()` to launch a coroutine into the server's main event loop.

```cpp
Async<void> price_ticker(App& app) {
    while (true) {
        auto price = co_await fetch_current_price();
        app.broadcast("/ticker", Json({{"price", price}}));

        // Wait for 1 second WITHOUT blocking any threads
        co_await blaze::delay(std::chrono::seconds(1));
    }
}

// In main()
app.spawn(price_ticker(app));
```

---

## 4. Automatic Pruning & Health

Web connections are fragile. Blaze uses intelligent session management to keep your server clean.

### Dead Connection Pruning
Blaze uses `std::weak_ptr` to track WebSocket sessions in the broadcast registry. If a client's internet drops or their browser closes, Blaze automatically detects the "dead" connection during the next broadcast and prunes it from the registry. You don't have to worry about memory leaks from stale connections.

### Heartbeats (Ping/Pong)
Blaze's internal engine handles the WebSocket Ping/Pong control frames automatically. This keeps connections alive through load balancers and firewalls that might otherwise close idle sockets. No manual configuration is required.

---

## 5. Async Utilities

### `blaze::delay`
Never use `std::this_thread::sleep_for` in a web framework! It freezes the entire worker thread. Instead, use `co_await blaze::delay`. It tells the framework: "I'm pausing this specific coroutine for X milliseconds; go ahead and handle other users while I wait."
