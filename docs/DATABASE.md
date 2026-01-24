# Database Drivers & ORM

Blaze provides a modular, asynchronous database abstraction layer that supports PostgreSQL and MySQL/MariaDB.

## 1. Modular Drivers
To keep binaries lean, database drivers are not linked by default. You must add them to your project using the Blaze CLI.

```bash
# For PostgreSQL
blaze add postgres

# For MySQL / MariaDB
blaze add mysql
```

The CLI will verify your system dependencies and update your `CMakeLists.txt` automatically.

## 2. Object Mapping (BLAZE_MODEL)
Blaze uses the `BLAZE_MODEL` macro to enable automatic serialization between database rows, C++ structs, and JSON.

```cpp
struct Product {
    int id;
    std::string name;
    double price;
};

// Enables mapping for id, name, and price
BLAZE_MODEL(Product, id, name, price)
```

## 3. Connecting to a Database
Use the `service` registry to initialize a connection pool and register it in the DI container.

```cpp
// PostgreSQL
app.service(Postgres::open(app, "postgresql://user:pass@localhost:5432/db", 10))
   .as<Database>();

// MySQL
app.service(MySql::open(app, "mysql://user:pass@localhost:3306/db", 10))
   .as<Database>();
```

## 4. Executing Queries

### Type-Safe Results
When using a model, Blaze automatically maps the result rows to a `std::vector` of your objects.

```cpp
app.get("/products", [](Database& db) -> Async<std::vector<Product>> {
    // Returns std::vector<Product>
    auto results = co_await db.query<Product>("SELECT id, name, price FROM products");
    co_return results;
});
```

### Parameterized Queries
Always use parameters to prevent SQL injection. Blaze supports both positional (`?`) and indexed (`$1`) syntax depending on the driver.

```cpp
// Postgres syntax ($1, $2)
auto users = co_await db.query<User>("SELECT * FROM users WHERE email = $1", {"alice@example.com"});

// MySQL syntax (?)
auto users = co_await db.query<User>("SELECT * FROM users WHERE email = ?", {"alice@example.com"});
```

### Dynamic Access
If you don't need a model, you can access columns by name or index.

```cpp
auto result = co_await db.query("SELECT count(*) as total FROM users");
int count = result[0]["total"].as<int>();
```

## 5. Performance Features
*   **Connection Pooling**: Managed automatically by the framework.
*   **Circuit Breaking**: Blaze automatically stops sending queries if the database becomes unresponsive, preventing application "Death Spirals."
*   **Async/Await**: No threads are blocked during I/O operations.
