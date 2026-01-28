# Dependency Injection

Dependency Injection (DI) is a design pattern that allows you to write decoupled, testable code. Instead of a class creating its own dependencies (like a database connection), the framework "injects" them.

---

## 1. Why use Dependency Injection?

Imagine your `UserService` needs a `Database`. Without DI, you might hardcode a connection string inside the service. With DI:
1.  You define what the service needs.
2.  The framework provides the dependency at runtime.
3.  **Testing**: You can easily swap the real database for a "mock" during unit tests.

---

## 2. Registering Services

Services are registered in the `App` instance, usually inside your `main()` function.

### Singleton (One instance for the whole app)
By default, `provide<T>` creates one instance and shares it everywhere.

```cpp
// Blaze creates the instance for you
app.provide<EmailService>();

// You can also pass constructor arguments
app.provide<ConfigService>("config.json");
```

### Transient (New instance every time)
If you need a fresh object every time it's requested, use `provide_transient`.

```cpp
app.provide_transient<HelperTool>();
```

### Interface Mapping
This is a best practice. It allows you to depend on an **interface** rather than a **concrete implementation**.

```cpp
// Register a Postgres pool as the "Database" service
app.service(Postgres::open(app, "..."))
   .as<Database>();
```

---

## 3. Constructor Injection (`BLAZE_DEPS`)

To tell Blaze that a class has dependencies, use the `BLAZE_DEPS` macro. Blaze will automatically resolve these from the container and pass them to your constructor.

```cpp
class UserService {
public:
    // 1. Declare dependencies
    BLAZE_DEPS(Database, Logger)
    
    // 2. Receive them as shared_ptrs
    UserService(std::shared_ptr<Database> db, std::shared_ptr<Logger> log)
        : db_(db), log_(log) {}

    void register_user(std::string name) {
        log_->info("Registering: " + name);
        db_->execute("INSERT INTO users...");
    }

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<Logger> log_;
};
```

---

## 4. Magic Handler Injection

Registered services can also be injected directly into your route handlers. This makes your route logic incredibly clean.

```cpp
app.get("/users/count", [](UserService& users) -> Async<Json> {
    // 'users' is automatically resolved and injected!
    int count = co_await users.get_total_count();
    co_return Json({{"count", count}});
});
```

### How does it work?
When a request comes in, Blaze's "Brain" (the `ServiceProvider`) looks at the handler's arguments. It sees you want a `UserService`, finds it in the container, and hands it to the function.

---

## 5. Lifetimes & Ownership

*   **Shared Ownership**: All DI services are managed using `std::shared_ptr`. You don't need to worry about `delete` or memory leaks.
*   **Thread Safety**: The DI container is thread-safe. You can resolve services from multiple threads simultaneously.
*   **Auto-Wiring**: If a service you're providing itself has dependencies (via `BLAZE_DEPS`), Blaze will recursively resolve the entire chain for you.
