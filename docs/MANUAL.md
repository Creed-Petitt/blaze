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

### Registering Services (Fluent API)
In `main()`, you can register services using the fluent `app.service()` syntax:

```cpp
// Register a specific service
app.service(std::make_shared<EmailService>());

// Register as an Interface (e.g., Postgres implementing Database)
app.service(Postgres::open(app, "...", 10))
   .as<Database>();

// Transient: Created new every time (Use low-level API)
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

## 4. Database & Mini-ORM

Blaze provides a unified `Database` interface with built-in Object Mapping (`BLAZE_MODEL`).

### Defining Models
Use the `BLAZE_MODEL` macro to auto-generate serialization code for your structs.

```cpp
struct User {
    int id;
    std::string name;
    std::string email;
};
// Magic macro enabling JSON and DB conversion
BLAZE_MODEL(User, id, name, email)
```

### Type-Safe Queries
You can query the database and get strongly-typed objects back automatically.

```cpp
// Returns std::vector<User>
auto users = co_await db->query<User>("SELECT * FROM users WHERE active = $1", {"true"});

if (users.empty()) throw NotFound("User not found");

res.json(users); // Automatically serialized to JSON array
```

### Dynamic Access (No Model)
If you don't define a model, you can access columns dynamically using the clean Value Wrapper API.

```cpp
auto result = co_await db->query("SELECT count(*) as count FROM users");
int count = result[0]["count"].as<int>();
```

### Setup (Postgres & MySQL)
Blaze supports swapping drivers with zero code changes.

```cpp
// Use Postgres
app.service(Postgres::open(app, "postgresql://...", 10))
   .as<Database>();

// OR Use MySQL
app.service(MySql::open(app, "mysql://...", 10))
   .as<Database>();
```

## 5. JSON Handling & Validation

Blaze uses **Boost.JSON** for maximum performance, wrapped in a type-safe API.

### Receiving JSON
Use `req.json<T>()` to parse incoming bodies directly into your Models.
This method **automatically throws** `BadRequest` if the JSON is malformed or types mismatch.

```cpp
app.post("/users", [](Request& req, Response& res) {
    // If body is invalid, this throws 400 Bad Request immediately
    auto user = req.json<User>();
    
    save_to_db(user);
    
    res.status(201).json(user);
});
```

### Exceptions
Blaze provides standard HTTP exceptions for clean control flow.

```cpp
if (id < 0) throw BadRequest("Invalid ID");
if (!found) throw NotFound("Item not found");
throw InternalServerError("Something went wrong");
```

## 6. WebSockets

Blaze includes a high-performance, thread-safe WebSocket server built on Beast.

### Basic Usage
Register a WebSocket route using `app.ws()`.

```cpp
app.ws("/chat", {
    .on_open = [](std::shared_ptr<WebSocket> ws) {
        std::cout << "Client connected!" << std::endl;
    },
    .on_message = [](std::shared_ptr<WebSocket> ws, std::string msg) {
        ws->send("You said: " + msg);
    }
});
```

## 7. Docker & Deployment

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