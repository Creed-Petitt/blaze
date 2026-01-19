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

## 3. Dependency Injection (IoC)

Blaze features a robust, thread-safe Dependency Injection container built-in.

### Registering Services
In `main()`, you can register services before starting the app:

```cpp
// Singleton: Created once, shared everywhere
app.provide<Database>(my_pg_pool);

// Factory Singleton: Created lazily when first needed
app.provide<Config>([](ServiceProvider& sp) {
    return std::make_shared<Config>("config.json");
});

// Transient: Created new every time
app.provide_transient<Logger>();
```

### Auto-Wiring (`BLAZE_DEPS`)
You can define dependencies directly in your class using the `BLAZE_DEPS` macro. Blaze will automatically resolve and inject them into the constructor.

```cpp
class UserService {
public:
    // "I need a Database and a Config"
    BLAZE_DEPS(Database, Config)

    UserService(std::shared_ptr<Database> db, std::shared_ptr<Config> cfg) 
        : db_(db), cfg_(cfg) {}
    
    // ...
};
```

### Route Injection
Blaze supports "Magic Injection" in route handlers. You can request any registered service as an argument.

```cpp
app.get("/users", [](Response& res, UserService& user) -> Task {
    co_await user.get_all(res);
});
```

## 4. Database Drivers

Blaze provides native drivers and a unified `Database` interface.

### The `Database` Interface
We recommend coding against the `blaze::Database` interface to keep your code portable.

```cpp
// Works with BOTH Postgres and MySQL
auto res = co_await db->query("SELECT * FROM users");
```

### Setup (Postgres)
```cpp
auto pg = std::make_shared<PgPool>(app.engine(), "postgres://user:pass@host/db", 10);
pg->connect();
app.provide<Database>(pg);
```

### Setup (MySQL)
```cpp
auto mysql = std::make_shared<MySqlPool>(app.engine(), "mysql://user:pass@host:3306/db", 10);
mysql->connect();
app.provide<Database>(mysql);
```

## 5. JSON Handling

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

## 7. WebSockets

Blaze includes a high-performance, thread-safe WebSocket server built on Beast.

### Basic Usage
Register a WebSocket route using `app.ws()`. You can handle Open, Message, and Close events.

```cpp
app.ws("/chat", {
    .on_open = [](std::shared_ptr<WebSocket> ws) {
        std::cout << "Client connected!" << std::endl;
    },
    .on_message = [](std::shared_ptr<WebSocket> ws, std::string msg) {
        // Echo the message back
        ws->send("You said: " + msg);
    },
    .on_close = [](std::shared_ptr<WebSocket> ws) {
        std::cout << "Client disconnected." << std::endl;
    }
});
```

### Thread Safety
The `ws->send()` method is thread-safe. You can call it from any thread or async callback (e.g., from a database query result or a game loop).

## 8. Docker & Deployment

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
