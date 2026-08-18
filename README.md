<div align="center">
  <img src="docs/assets/logo.png" width = 500>
  <p><b>Blaze is a high-performance, asynchronous C++20 web framework designed for developer productivity and extreme scalability</b></p>


[![Blaze CI](https://github.com/Creed-Petitt/blaze/actions/workflows/ci.yml/badge.svg)](https://github.com/Creed-Petitt/blaze/actions/workflows/ci.yml)
![License](https://img.shields.io/badge/license-MIT-red)
![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macos-red)
![Standard](https://img.shields.io/badge/c%2B%2B-20-red)

</div>

## Features

- **Asynchronous Coroutines**: High-concurrency powered by C++20 `co_await`.
- **Type-Safe Extraction**: Automatic injection of `Path<T>`, `Body<T>`, and `Query<T>`.
- **Explicit Services**: Register and resolve shared services without hiding application wiring.
- **Focused Middleware**: Rate limiting, CORS, static files, and request context.
- **High-Efficiency WebSockets**: Scalable real-time communication.
- **Automatic Validation**: Integrity checks via `validate()` hooks.

## Installation

Install Blaze as a CMake package, or consume it directly with `FetchContent`.

```bash
// YOLO mode
curl -fsSL https://raw.githubusercontent.com/Creed-Petitt/blaze/main/install.sh | bash
```

> **Note:** The installer (`install.sh`) installs only the core build tools.

## Local Development

Build Blaze from this repository:

```bash
./build.sh
```

Build and run the test suite:

```bash
./build.sh test
```

## Quick Start

1. **Write** your logic (e.g., in `src/main.cpp`):
   ```cpp
   #include <blaze/app.h>

   using namespace blaze;

   int main() {
       App app;

       app.get("/hello", [](Response& res) -> Async<void> {
           res.send("Hello from Blaze");
           co_return;
      });

       app.listen(8080);
   }
   ```

2. **Build and run** with CMake:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel
   ./build/my-api
   ```

## Documentation

### Project Setup
- **[Getting Started](docs/getting-started.md)**: Installation and project setup.
- **[Build System](docs/build-system.md)**: Direct CMake usage and install/package flow.
- **[Configuration](docs/configuration.md)**: The App Builder API, logging, and environment variables.
- **[Architecture & Design](docs/architecture.md)**: Deep dive into the framework's core engine and design patterns.
- **[Testing & Security](docs/testing.md)**: CI/CD, sanitizers, and performance benchmarking.

### Core Framework
- **[Routing & Request Handling](docs/routing.md)**: Methods, parameters, and typed injection.
- **[Services](docs/dependency-injection.md)**: Explicit app-owned service registration.
- **[File Uploads](docs/file-uploads.md)**: Multipart forms and client-side uploads.
- **[Middleware](docs/middleware.md)**: CORS, rate limiting, static files, and the middleware chain.
- **[WebSockets](docs/websockets.md)**: Real-time broadcasting and background tasks.
- **[Core Utilities](docs/utilities.md)**: Zero-allocation tools for strings, parsing, and resilience.

## Requirements

Blaze is designed for modern Linux and macOS environments.

- **Compiler**: C++20 compliant (GCC 11+, Clang 13+, or MSVC 2022+).
- **Engine**: Boost 1.85+ (Automatically managed via CMake).

## License

Blaze is released under the [MIT License](LICENSE).
