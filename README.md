# Blaze 

![Platform](https://img.shields.io/badge/os-linux%20%7C%20macos-blue)
![C++](https://img.shields.io/badge/c%2B%2B-20-blue)
![Status](https://img.shields.io/badge/status-beta-yellow)

**Blaze** is a high-performance, zero-config C++20 web framework built for extreme and developer simplicity. It combines an internalized, non-blocking engine with an elegant API to build the fastest backends on the planet.

## Requirements

Blaze **brings its own engine** (Boost 1.85.0). You only need the following system libraries:

*   **CMake** (3.20+)
*   **G++ / Clang** (C++20 support)
*   **OpenSSL** (`libssl-dev`)
*   **libpq** (`libpq-dev`) - *Optional (Required only for the PostgreSQL driver)*
*   **libmysqlclient** (`libmysqlclient-dev`) - *Optional (Required only for MySQL)*


## Features

*   **Internalized Engine**: Zero system dependencies. Blaze builds its own IO and Threading backends.
*   **Blazing Fast**: Handles **53,000+ req/sec** (plaintext) and **45,000+ req/sec** (JSON).
*   **Async-First**: Built on C++20 Coroutines (`co_await`).
*   **Multi-Driver Support**: Native, non-blocking drivers for **PostgreSQL**, **Redis**, and **MySQL**.
*   **Modern TUI**: A beautiful interface for builds and scaffolding.
*   **Modular Docker**: Built-in commands to manage app containers and background databases.

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

## Modular Docker CLI
Blaze manages your development environment for you.

```bash
blaze docker psql     # Start background Postgres
blaze docker redis    # Start background Redis
blaze docker mysql    # Start background MySQL
blaze docker logs     # View database logs
blaze docker build    # Build app image
blaze docker run      # Run app in container
blaze docker stop     # Cleanup all containers
```

## Quick Start

Write an API in just 10 lines of code.

```cpp
#include <blaze/app.h>
using namespace blaze;

int main() {
    App app;

    // Hello World
    app.get("/hello", [](Request& req, Response& res) -> Task {
        res.json({
            {"message", "Hello from Blaze!"},
            {"timestamp", std::time(nullptr)}
        });
        co_return;
    });

    app.listen(8080);
    return 0;
}
```

## Performance Metrics

Blaze is engineered for maximum throughput on modern hardware.

| Metric | Performance          |
| :--- |:---------------------|
| **Plaintext Throughput** | **53,821 req/sec**   |
| **JSON Serialization** | **45,794 req/sec**   |
| **PostgreSQL Latency** | **21,376 trans/sec** |
| **MySQL Latency** | **14,200 trans/sec** |
| **Redis IOPS** | **50,000+ ops/sec**  |

*Hardware: Intel Core i7-1165G7 @ 4.70GHz, 12GB RAM, Ubuntu 22.04. Benchmark: `wrk -t8 -c100 -d10s`.*

## Roadmap to v1.0

Blaze is currently in **Beta (v0.3.0)**.

- [x] **Core**: High-performance Async Server & Router.
- [x] **CLI**: Modern TUI with progress tracking and "Ignite" effects.
- [x] **Engine**: Internalized Boost 1.85.0 dependency model.
- [x] **Drivers**: Non-blocking PostgreSQL, Redis, and MySQL.
- [ ] **Step 8**: WebSockets Support (Real-time communication).
- [ ] **Step 9**: App & Beast Engine Tuning (io_uring, Threading).
- [ ] **Step 10**: Secure Hashing Polish (Timing-attack resistance).
- [ ] **Step 11**: Environment & Secret Manager (.env support).
- [ ] **Step 12**: Recovery & Logging Middleware.
- [ ] **Step 13**: Testing Framework & HTTP/2 Support.
- [ ] **Step 14**: Dependency Injection Manager (IoC).
- [ ] **Step 15**: Cloud Infra & SDKs (AWS/GCP/Terraform).
- [ ] **Step 16**: **Blaze ORM** (v2.0 Flagship).

## License

[MIT](LICENSE)
