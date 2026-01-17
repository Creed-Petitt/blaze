# Blaze Framework Manual

Welcome to **Blaze**, the high-performance C++20 web framework.

## 1. Project Structure

A typical Blaze project (created via `blaze init`) is self-contained:

```
my-app/
├── CMakeLists.txt       # Fetches Blaze and internal Boost engine
├── Dockerfile           # Multi-stage production build
├── docker-compose.yml   # Modular dev environment (Postgres/Redis/MySQL)
└── src/
    └── main.cpp         # Application entry point
```

## 2. The Blaze CLI

The CLI is your primary tool for development.

- `blaze init <name>`: Scaffolds a new project.
- `blaze run`: Builds and runs the project with a progress TUI.
- `blaze docker <service>`: Manages background databases (e.g., `blaze docker psql`).

## 3. Database Drivers

Blaze provides three native, non-blocking drivers with built-in connection pooling.

### PostgreSQL
```cpp
#include <blaze/postgres.h>
Postgres db(app, "postgres://user:pass@host/db", 10);
auto res = co_await db.query("SELECT * FROM users");

for (auto row : res) {
    std::cout << row["name"].as<std::string>() << "\n";
}
```

### Redis
```cpp
#include <blaze/redis.h>
Redis redis(app, "localhost", 6379);
co_await redis.set("key", "value");
std::string val = co_await redis.get("key");
```

### MySQL
```cpp
#include <blaze/mysql.h>
MySql mysql(app, "mysql://user:pass@host:3306/db", 10);
auto res = co_await mysql.query("SELECT * FROM table");

for (auto row : res) {
    std::cout << row["name"].as<std::string>() << "\n";
}
```

## 4. JSON Handling

Blaze uses **Boost.JSON** for maximum performance.

```cpp
// Sending JSON
res.json({
    {"status", "success"},
    {"data", {1, 2, 3}}
});

// Receiving JSON
app.post("/api/data", [](Request& req, Response& res) -> Task {
    try {
        auto data = req.json(); // returns blaze::Json wrapper
        std::string name = data["name"].as<std::string>();
        int id = data["id"].as<int>();
        
        res.status(201).json({{"created", name}, {"id", id}});
    } catch (const std::exception& e) {
        res.bad_request("Invalid JSON");
    }
    co_return;
});
```

## 5. Middleware

Middleware functions are fully asynchronous (`Task`). They can modify the request/response or halt execution.

```cpp
Middleware logger = [](Request& req, Response& res, auto next) -> Task {
    auto start = std::chrono::steady_clock::now();
    co_await next();
    auto end = std::chrono::steady_clock::now();
    // Log duration...
};

app.use(logger);
```

## 6. Docker & Deployment

Blaze supports modular deployment through the CLI.

### Local Development with Docker
Start only the databases you need:
```bash
blaze docker redis
blaze docker psql
blaze run
```

### Containerized Deployment
Run the entire stack in isolated containers:
```bash
# Builds the app and starts all dependencies
docker compose up --build
```

### Production Build
Blaze uses multi-stage builds to create tiny, high-performance images (~40MB) that contain only the static binary and minimal runtime libraries.