# High-Performance C++ HTTP Framework

A lightweight HTTP/1.1 framework built from scratch in C++ featuring multi-reactor event loops, Express-style routing, and middleware support. Built using edge-triggered epoll and POSIX sockets, achieving **70,000+ req/s** sustained throughput.

## About

Combines the simplicity of Express.js with the performance of low-level systems programming. Built entirely from scratch without external dependencies (except JSON parsing), it demonstrates modern C++ techniques for high-concurrency network servers.

**Key Architecture:**
- **Multi-reactor pattern**: 8 independent event loops (one per CPU core) using `SO_REUSEPORT` for load balancing
- **Edge-triggered epoll**: Non-blocking I/O with efficient kernel event notification
- **Bounded thread pool**: Request handlers execute off the event loop with backpressure control
- **HTTP/1.1 keep-alive**: Connection reuse and request pipelining for maximum throughput

## Performance

Benchmarked with [wrk](https://github.com/wg/wrk) on 8-core system:

```bash
wrk -t8 -c200 -d120s http://localhost:8080/
```

**Results:**
- **50-70k requests/sec** sustained
- **5 million requests** in 90 seconds
- **<5ms average latency** under load
- **HTTP keep-alive** with pipelined requests

## Features

- **Express.js-style API**: Familiar routing with `app.get()`, `app.post()`, `app.put()`, `app.del()`
- **Route Parameters**: Dynamic paths like `/users/:id` with automatic extraction
- **Route Grouping**: Organize endpoints with shared prefixes (`/api/v1`)
- **Middleware Pipeline**: Composable request/response interceptors
- **JSON Support**: Built-in parsing and serialization
- **Static File Serving**: Middleware for serving HTML, CSS, JS with proper MIME types
- **CORS Support**: Pre-built middleware for cross-origin requests
- **Graceful Shutdown**: Clean termination with signal handling

## Quick Example

```cpp
#include "framework/app.h"

int main() {
    App app;

    // Simple route
    app.get("/", [](Request& req, Response& res) {
        res.json({{"status", "ok"}});
    });

    // Route with parameters
    app.get("/users/:id", [](Request& req, Response& res) {
        auto id = req.get_param_int("id");
        res.json({{"user_id", id.value_or(0)}});
    });

    // Route grouping
    auto api = app.group("/api/v1");
    api.get("/health", [](Request& req, Response& res) {
        res.status(200).send("healthy\n");
    });

    app.listen(8080);  // Starts 8 event loops automatically
    return 0;
}
```

## How It Works

Uses **multi-reactor architecture** for high concurrency:

1. **Multiple Event Loops**: Each CPU core runs its own independent `epoll` event loop in a dedicated thread
2. **Load Balancing**: The kernel distributes incoming connections across event loops using `SO_REUSEPORT`
3. **Non-blocking I/O**: Edge-triggered epoll with `EPOLLET` minimizes syscalls and maximizes throughput
4. **Async Request Handling**: Each event loop dispatches requests to a shared thread pool for handler execution
5. **Lock-free Design**: Each event loop owns its connections map with no mutex contention

This design scales linearly with CPU cores while maintaining sub-millisecond latency.

## API Overview

| Component | Purpose |
|-----------|---------|
| `App` | Main application class for routes and middleware |
| `Request` | HTTP request with headers, params, query strings, and JSON body |
| `Response` | Chainable response builder (`status()`, `header()`, `send()`, `json()`) |
| `Router` | Pattern matching for routes with parameter extraction |
| `RouteGroup` | Organize routes under shared prefixes |
| `middleware::` | Pre-built CORS, static files, and body size limits |

<details>
<summary><b>Build Instructions</b></summary>

### Prerequisites
- C++17 or later
- CMake 3.10+
- Linux/Unix (uses `epoll` and POSIX sockets)

### Build
```bash
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./http_server
```

Server starts on `http://localhost:8080`

</details>

<details>
<summary><b>Docker Setup</b></summary>

### Using Docker Compose
```bash
docker-compose up --build
```

### Manual Docker Build
```bash
docker build -t http-server .
docker run -p 8080:8080 http-server
```

</details>

## Project Structure

```
http_server/
├── framework/
│   ├── app.h/.cpp           # Application, routing, middleware
│   ├── HttpServer.h/.cpp    # Multi-reactor event loops (epoll)
│   ├── router.h/.cpp        # Route matching and parameters
│   ├── request.h/.cpp       # HTTP request parsing
│   ├── response.h/.cpp      # HTTP response building
│   ├── middleware.h         # CORS, static files, body limits
│   └── logger.h             # Access and error logging
├── thread_pool.h            # Bounded thread pool with backpressure
├── main.cpp                 # Example server with routes
├── json.hpp                 # JSON library (nlohmann/json)
└── CMakeLists.txt
```
