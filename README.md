# Blaze 

![Platform](https://img.shields.io/badge/os-linux%20%7C%20macos-blue)
![C++](https://img.shields.io/badge/c%2B%2B-20-blue)
![Status](https://img.shields.io/badge/status-beta-yellow)

**Blaze** is a high-performance, zero-config C++20 web framework built for peak developer experience. It combines an internalized, non-blocking engine with an elegant API to build the fastest backends on the planet.

## Requirements

Blaze **brings its own engine** (Boost 1.85.0). You only need the following system libraries:

*   **CMake** (3.20+)
*   **G++ / Clang** (C++20 support)
*   **OpenSSL** (`libssl-dev`)
*   **libpq** (`libpq-dev`) - *Required for Database Interface*
*   **libmariadb** (`libmariadb-dev`) - *Required for Database Interface*

> **Note:** The installer (`install.sh`) handles these dependencies automatically for you.

## Features

*   **Blazing Fast**: Handles **176,000+ req/sec** (plaintext) and **140,000+ req/sec** (JSON).
*   **Dependency Injection (IoC)**: First-class support for Singletons, Transients, and Auto-Wiring (`BLAZE_DEPS`).
*   **Async-First**: Built on C++20 Coroutines (`co_await`).
*   **Real-Time WebSockets**: Built-in support for high-performance WebSocket connections.
*   **Database Agnostic**: Switch between Postgres and MySQL with **zero code changes** using the `Database` interface.
*   **Modern TUI**: A beautiful interface for builds and scaffolding.
*   **All-in-One**: Built-in commands to manage app containers and background databases.

## Installation

Install the **Blaze CLI** to scaffold and run projects with zero configuration.

### Option 1: Quick Install (Recommended)
```bash
curl -fsSL https://raw.githubusercontent.com/Creed-Petitt/blaze/main/install.sh | bash
```

### Option 2: Build from Source
```bash
git clone https://github.com/Creed-Petitt/blaze.git
cd blaze
./install.sh
```

### Usage
Once installed, create and run your first project:
```bash
# Create a new project
blaze init my-api

# Build and Run with TUI
cd my-api
blaze run
```

## CLI Reference

Blaze comes with a powerful CLI to manage your entire development lifecycle, including Docker containers for Postgres, MySQL, and Redis.

For a full list of commands, see the **[CLI Reference](docs/CLI.md)**.

## Quick Start

Create a high-performance API in less than 15 lines.

```cpp
#include <blaze/app.h>
using namespace blaze;

int main() {
    App app;

    // JSON Response
    app.get("/", [](Response& res) {
        res.json({{"message", "Blaze is fast!"}});
    });

    // Path Parameters
    app.get("/hello/:name", [](Request& req, Response& res) {
        res.send("Hello, " + req.params["name"]);
    });

    app.listen(8080);
}
```

## Proven Reliability

Blaze is engineered for stability under extreme conditions.

*   **Battle-Tested:** Survives **48-hour continuous stress tests** at 100% CPU saturation with **zero memory leaks**.
*   **Memory Safe:** Verified clean by **Google AddressSanitizer (ASan)** and **LeakSanitizer (LSan)**.
*   **Fuzz Tested:** Resilient against malformed packets and malicious payloads.
*   **Crash-Proof:** Native circuit breakers handle database failures instantly.

## Performance Benchmarks

Benchmarks run on a standard laptop (Localhost, 8 threads, 1000 connections).

| Framework | Lang | JSON Req/Sec | Latency |
|-----------|------|-------------:|--------:|
| **Blaze** | **C++20** |  **176,000** | **2.34ms** |
| Gin | Go |     ~140,000 | 8ms |
| Node.js | JS |      ~35,000 | 15ms |
| FastAPI | Python |      ~15,000 | 25ms |

## License

[MIT](LICENSE)