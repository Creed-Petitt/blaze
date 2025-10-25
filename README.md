# Concurrent HTTP Server


## How It Works

1. **Socket Creation**: Server binds to port 8080 and listens for connections
2. **Connection Acceptance**: Main loop accepts incoming client connections
3. **Task Dispatch**: Each connection is queued as a task in the thread pool
4. **Concurrent Processing**: Worker threads pick up tasks and handle HTTP requests
5. **Response**: Files are read, proper headers added, and sent back to client

## Architecture

- **Main Thread**: Accepts incoming connections and dispatches to thread pool
- **Worker Threads**: Process HTTP requests in parallel using producer-consumer pattern
- **Thread-Safe Queue**: Coordinates work distribution across threads with mutex/condition variables

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
├── main.cpp           # Server implementation and request handling
├── thread_pool.h      # Thread pool with producer-consumer pattern
├── logger.h           # Thread-safe logging with timestamps
├── public/            # Static files to serve
│   ├── index.html
│   ├── health.txt
│   └── ...
├── logs/              # Generated log files (created at runtime)
│   ├── access.log     # Request logs with timing
│   └── error.log      # Error logs
├── Dockerfile         # Docker container configuration
├── docker-compose.yml # Docker Compose configuration
└── CMakeLists.txt
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



