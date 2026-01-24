<div align="center">
  <img src="docs/assets/logo.png" alt="Blaze Logo" width="200">

  [![Blaze CI](https://github.com/Creed-Petitt/blaze/actions/workflows/ci.yml/badge.svg)](https://github.com/Creed-Petitt/blaze/actions/workflows/ci.yml)
  ![License](https://img.shields.io/badge/license-MIT-blue)
  ![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macos-lightgrey)
  ![Standard](https://img.shields.io/badge/c%2B%2B-20-blue)

  <p>
    <a href="#features">Features</a> •
    <a href="#installation">Installation</a> •
    <a href="#quick-start">Quick Start</a> •
    <a href="docs/MANUAL.md">Documentation</a>
  </p>
</div>

---

**Blaze** is a high-performance, zero-config C++20 web framework built for peak developer experience. It combines a non-blocking engine with an elegant API to build the fastest, cleanest backends on the web.

## Features

*   **150,000+ req/sec** (plaintext) and **130,000+ req/sec** (JSON) with sub 10 mb/s latency.
*   Write safe and clean handlers with **Auto-Injection** (`[](User u)`) and **Async Returns** (`-> Async<User>`).
*   First-class support for Singletons, Transients, and Auto-Wiring (`BLAZE_DEPS`).
*   Built solely for C++20 Coroutines (`co_await`).
*   Easy-to-use API for high-performance WebSocket connections.
*   Asynchronous PostgreSQL and MySQL drivers accessed via the `Database` interface.
*   Built-in commands to manage app containers and background databases.

## Requirements

Blaze is tested and verified to work out-of-the-box on:
*   **Ubuntu** (22.04, 24.04+)
*   **Fedora** (40, 41, 42+)
*   **Rocky Linux / AlmaLinux** (9+)
*   **macOS** (Apple Silicon/Intel)

You only need the following system libraries:
*   **CMake** (3.20+)
*   **G++ / Clang** (C++20 support)
*   **OpenSSL** (`libssl-dev` / `openssl-devel`)
*   **libpq** (`libpq-dev` / `libpq-devel`) ``//optional``
*   **libmariadb** (`libmariadb-dev` / `mariadb-devel`) ``//optional``

> **Note:** The installer (`install.sh`) handles these dependencies and repository setup (EPEL/CRB) automatically for you.

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
blaze init my-api --fullstack

# Build and Run with Unified Logs
cd my-api
cd frontend && npm install && cd ..
blaze dev
```

## Quick Start

Create a high-performance API in less than 15 lines.

```cpp
#include <blaze/app.h>
#include <blaze/model.h>
using namespace blaze;

struct User { 
    int id; 
    std::string name; 
};
BLAZE_MODEL(User, id, name)

int main() {
    App app;

    // Auto-Injection & Auto-Return
    app.post("/users", [](User user) -> Async<User> {
        if (user.id < 0) 
          throw BadRequest("Invalid ID");
        
        // Save to DB...
        
        co_return user; // Auto-serialized to JSON
    });

    app.listen(8080);
}
```

For a deep dive into the API, Dependency Injection, and WebSockets, see the **[Full Framework Manual](docs/MANUAL.md)**.

## CLI Reference

Blaze comes with a powerful CLI to manage your entire development lifecycle. Below is a few useful commands See the **[Full CLI Reference](docs/CLI.md)** for more details.

| Command | Description                                                       |
| :--- |:------------------------------------------------------------------|
| `init` | Scaffold a new project (add `--fullstack` for UI).                |
| `dev` | Run C++ and Vite in parallel with unified logs.                   |
| `add` | Add features like a `frontend` or `database` to existing projects. |
| `doctor` | Verify your system requirements.                                  |


## Testing and Security

Every commit is audited via an automated **CI pipeline** in GitHub Actions. Feel free to view **[Full Testing Pipeline](docs/TESTING.md)** for more info.

*   **Logic Integrity:** Comprehensive **Catch2** unit tests for every core component (Routing, DI, JSON).
*   **Memory Safety:** Automatic memory audit via **AddressSanitizer (ASan)** and **UBSan** on every push.
*   **Concurrency Safety:** Race condition and deadlock detection via **ThreadSanitizer (TSan)** on every push.
*   **Fuzz Tested:** Resilient against malformed packets and malicious payloads.
*   **Compatability:** Verified builds and tests on **Ubuntu Linux** and **macOS**.


## License

[MIT](LICENSE)