# Database & ORM

Blaze provides a modular database layer that combines high-performance asynchronous drivers with a reflection-based ORM (Object-Relational Mapper).

---

## 1. Setup & Connection

### Installing Drivers
To keep your application as light as possible, database drivers are not included by default. You can add them using the Blaze CLI:

```bash
blaze add postgres  # Adds PostgreSQL driver
blaze add mysql     # Adds MySQL/MariaDB driver
```

### Establishing a Connection
The easiest way to connect is using the static `install()` helper in your `main()` function. This automatically creates a connection pool and registers it as the default `Database` service.

```cpp
#include <blaze/postgres.h> // or <blaze/mysql.h>

int main() {
    App app;

    // Connect to Postgres (Pool size defaults to 10)
    Postgres::install(app, "postgresql://user:pass@localhost/db");
    
    // Or Connect to MySQL
    // MySql::install(app, "mysql://user:pass@localhost/db");

    app.listen(8080);
}
```

### Advanced: Manual Registration
If you need to manage the pool lifetime yourself or connect to multiple databases, you can use the manual service registration API.

```cpp
// Open a pool with 10 connections (but don't register it yet)
auto pool = Postgres::open(app, "postgresql://user:pass@localhost/db", 10);

// Register it as the 'Database' service for the whole app
app.service(pool).as<Database>();
```

### Fault Tolerance (Circuit Breaker)
Database drivers in Blaze are protected by an automatic **Circuit Breaker**. 
*   **Behavior**: If the database fails 5 times in a row, the breaker "trips".
*   **Safety**: For the next 5 seconds, all database requests will immediately fail without attempting to connect. This prevents your application from overwhelming a struggling database or hanging threads on dead sockets.
*   **Recovery**: After 5 seconds, it allows one "probe" request. If successful, the breaker resets.

---

## 2. Defining Models (`BLAZE_MODEL`)

A "Model" is a simple C++ struct that represents a table in your database. By using the `BLAZE_MODEL` macro, you enable Blaze to automatically map database rows to your struct.

```cpp
struct Product {
    int id;
    std::string name;
    double price;
};

// This macro creates the "bridge" between C++ and SQL
BLAZE_MODEL(Product, id, name, price)
```

---

## 3. The Smart Repository

The **Repository** is your primary tool for interacting with the database. It provides a clean, type-safe API for CRUD (Create, Read, Update, Delete) operations.

### Automatic Naming
Blaze is smart about table names. If you have a struct named `UserAccount`, Blaze will look for a table named `user_accounts` (Snake Case + Pluralized).

### Basic Usage
You can request a repository for any model directly in your route handler.

```cpp
app.get("/products/:id", [](Path<int> id, Repository<Product> repo) -> Async<Product> {
    // 1. Fetch by Primary Key
    auto p = co_await repo.find(id);
    
    // 2. Return the object (Automatically converted to JSON)
    co_return p;
});
```

### Standard API:
*   **`find(id)`**: Fetches a single record. Throws `NotFound` if missing.
*   **`all()`**: Returns all records as a `std::vector<T>`.
*   **`save(model)`**: Inserts a new record. 
    *   *Note:* If the Primary Key is `0` (int) or empty (string), it is excluded from the query to allow the database to auto-increment/generate the ID.
*   **`update(model)`**: Updates an existing record (uses the first field as the ID).
*   **`remove(id)`**: Deletes a record.
*   **`count()`**: Returns the total number of rows.

---

## 4. The Fluent Query Builder

For more complex logic, use the `.query()` method to start a fluent chain.

```cpp
app.get("/search", [](Query<SearchParams> s, Repository<Product> repo) -> Async<Json> {
    auto results = co_await repo.query()
        .where("price", ">", s.min_price)
        .where("active", "=", true)
        .order_by("price", "DESC")
        .limit(10)
        .all();
        
    co_return Json(results);
});
```

Blaze automatically handles **SQL Injection** protection by using parameterized queries under the hood.

---

## 5. Raw Queries & Transactions

Sometimes you need to write raw SQL for performance, complex joins, or transactions. You can use the `Database` service directly.

```cpp
app.get("/stats", [](Database& db) -> Async<Json> {
    // Parameterized for safety!
    auto res = co_await db.query("SELECT count(*) as total FROM products WHERE price > $1", 100.0);
    
    int total = res[0]["total"].as<int>();
    co_return Json({{"high_value_items", total}});
});
```

### Transactions
Blaze provides a managed Transaction API that handles rollbacks automatically using RAII (Resource Acquisition Is Initialization) concepts.

```cpp
app.post("/transfer", [](Database& db) -> Async<Json> {
    // Start a transaction scope
    // "tx" is a special connection wrapper locked to this transaction
    co_await db.transaction([](Database& tx) -> Async<void> {
        
        // Use 'tx' for all queries in this block
        co_await tx.query("UPDATE accounts SET balance = balance - 100 WHERE id = 1");
        co_await tx.query("UPDATE accounts SET balance = balance + 100 WHERE id = 2");

        // If any exception is thrown here, the transaction automatically ROLLBACKs.
        // If the block completes successfully, it automatically COMMITs.
    });

    co_return Json({{"status", "success"}});
});
```

#### Auto-Injection in Transactions
For a cleaner syntax, you can ask for Repositories directly in the transaction lambda. Blaze will automatically bind them to the transaction's connection.

```cpp
app.post("/create-order", [](Database& db, Body<Order> order) -> Async<void> {
    
    // Inject Repositories directly!
    co_await db.transaction([&](Repository<Order> orders, Repository<Log> logs) -> Async<void> {
        
        // 'orders' and 'logs' are already connected to the transaction
        co_await orders.save(order);
        co_await logs.save({.msg = "Order created"});
        
    });
});
```
