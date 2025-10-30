# Concurrent HTTP Server

A lightweight, high-performance HTTP framework built in C++ with routing, concurrent request handling, and built-in JSON support.

## Features

- **Express.js-style API**: Intuitive routing with `app.get()`, `app.post()`, `app.put()`, `app.del()`
- **Dynamic Route Parameters**: Extract path variables like `/users/:id`
- **Query String Parsing**: Automatic parsing of URL query parameters
- **JSON Support**: Built-in JSON request/response handling with nlohmann/json
- **Concurrent Processing**: Thread pool with producer-consumer pattern for handling multiple requests
- **Request/Response Objects**: Clean API for working with HTTP requests and responses
- **Logging**: Thread-safe access and error logging with timestamps
- **Graceful Shutdown**: Signal handling for clean server termination

## Quick Example

```cpp
App app;

// Simple route
app.get("/hello", [](Request& req, Response& res) {
    res.send("Hello, World!\n");
});

// Route with parameters
app.get("/users/:id", [](Request& req, Response& res) {
    res.send("User ID: " + req.params["id"] + "\n");
});

// Query parameters
app.get("/search", [](Request& req, Response& res) {
    res.send("Query: " + req.query["q"] + "\n");
});

// JSON response
app.get("/api/data", [](Request& req, Response& res) {
    json data = {{"name", "John"}, {"age", 30}};
    res.json(data);
});

// POST with body
app.post("/users", [](Request& req, Response& res) {
    res.status(201).send("User created! Body: " + req.body + "\n");
});
```

## How It Works

1. **Socket Creation**: Server binds to port 8080 and listens for connections
2. **Connection Acceptance**: Main loop accepts incoming client connections
3. **Task Dispatch**: Each connection is queued as a task in the thread pool
4. **Request Parsing**: Request object parses HTTP method, path, headers, query params, and body
5. **Route Matching**: Router matches request to registered route handlers
6. **Handler Execution**: Worker threads execute matched route handler
7. **Response Building**: Response object constructs HTTP response with proper headers
8. **Logging**: Request details and timing logged to access.log

## Architecture

- **Framework Layer**:
  - `App`: Main application class with routing methods
  - `Router`: Pattern matching for routes with parameter extraction
  - `Request`: HTTP request parser with headers, body, params, and query
  - `Response`: Chainable response builder with status, headers, and body
- **Concurrency Layer**:
  - **Main Thread**: Accepts incoming connections and dispatches to thread pool
  - **Worker Threads**: Process HTTP requests in parallel using producer-consumer pattern
  - **Thread-Safe Queue**: Coordinates work distribution across threads with mutex/condition variables
- **Logging Layer**: Thread-safe logger with separate access and error logs

## Getting Started

### Prerequisites

- C++11 or later
- CMake 3.10+
- Linux/Unix environment (uses POSIX sockets)

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./http_server
```

Server starts on `http://localhost:8080`

### Docker (Alternative)

Build and run using Docker:

```bash
# Build the image
docker build -t http-server .

# Run the container
docker run -p 8080:8080 http-server

# Or use Docker Compose
docker-compose up
```

Server will be accessible at `http://localhost:8080`

### Project Structure

```
http_server/
├── framework/
│   ├── app.h/.cpp         # Application class with routing API
│   ├── router.h/.cpp      # Route matching and parameter extraction
│   ├── request.h/.cpp     # HTTP request parser
│   └── response.h/.cpp    # HTTP response builder
├── main.cpp               # Server setup and example routes
├── thread_pool.h          # Thread pool with producer-consumer pattern
├── logger.h               # Thread-safe logging with timestamps
├── json.hpp               # nlohmann/json library for JSON support
├── public/                # Static files directory (optional)
├── logs/                  # Generated log files (created at runtime)
│   ├── access.log         # Request logs with timing
│   └── error.log          # Error logs
├── Dockerfile             # Docker container configuration
├── docker-compose.yml     # Docker Compose configuration
└── CMakeLists.txt         # Build configuration
```

## Testing Performance

### Basic Test
```bash
curl http://localhost:8080/health.txt
```

### Load Testing with Apache Bench

Install Apache Bench:
```bash
sudo apt-get install apache2-utils
```

Run performance test:
```bash
# 1,000 requests with 100 concurrent connections
ab -n 1000 -c 100 http://localhost:8080/health.txt

# Stress test: 10,000 requests with 500 concurrent
ab -n 10000 -c 500 http://localhost:8080/health.txt
```

### Expected Results
- **Throughput**: 3,600+ requests/second
- **Latency**: ~26ms average response time
- **Concurrency**: Handles 500+ simultaneous connections
- **Reliability**: 0% failure rate under load

## Viewing Logs

All requests and errors are automatically logged to the `logs/` directory:

```bash
# View access logs (all requests with timing)
cat logs/access.log

# Example output:
# [2025-10-25 00:20:39] 127.0.0.1 GET /health.txt 200 4ms
# [2025-10-25 00:20:57] 127.0.0.1 GET /missing.txt 404 1ms
# [2025-10-25 00:21:06] 127.0.0.1 GET /api/data.json 200 2ms

# View error logs
cat logs/error.log

# Monitor logs in real-time
tail -f logs/access.log
```

**Log Format:**
- Timestamp in `YYYY-MM-DD HH:MM:SS` format
- Client IP address
- HTTP method (GET, POST, etc.)
- Requested path
- Status code (200, 404, 403, etc.)
- Response time in milliseconds



