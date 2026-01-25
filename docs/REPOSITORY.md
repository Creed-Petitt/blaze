# Smart Repository & Query Builder

Blaze features a powerful **Smart Repository** system. It acts as a lightweight ORM that automates common database operations while maintaining maximum performance.

## 1. Getting Started

A Repository is automatically available for any class defined with `BLAZE_MODEL`. You don't need to register it; just request it in your handler.

```cpp
struct User {
    int id;
    std::string name;
};
BLAZE_MODEL(User, id, name)

app.get("/users/:id", [](Path<int> id, Repository<User> repo) -> Async<User> {
    // Auto-injected and ready to use
    co_return co_await repo.find(id);
});
```

## 2. CRUD Operations

The `Repository<T>` provides standard methods for common tasks:

| Method | Description |
| :--- | :--- |
| `find(id)` | Fetches a record by Primary Key. Throws `NotFound` if missing. |
| `all()` | Returns a `std::vector<T>` of all records. |
| `save(model)` | Inserts a new record. |
| `update(model)` | Updates an existing record (matches by the first field). |
| `remove(id)` | Deletes a record by Primary Key. |
| `count()` | Returns the total row count as an `int`. |

## 3. Fluent Query Builder

For complex filters, use the `.query()` method to start a fluent chain. The builder handles SQL injection safety and placeholder indexing (`$1`, `$2`) automatically.

```cpp
app.get("/search", [](Query<SearchReq> req, Repository<User> repo) -> Async<Json> {
    auto users = co_await repo.query()
        .where("name", "=", req.name)
        .where("active", "=", true)
        .order_by("created_at", "DESC")
        .limit(10)
        .all();
        
    co_return Json(users);
});
```

### Supported Methods:
*   `.where(column, operator, value)`
*   `.order_by(column, direction)` (e.g., "ASC", "DESC")
*   `.limit(count)`
*   `.offset(count)`
*   `.all()`: Executes and returns a vector.
*   `.first()`: Returns the first match or throws `NotFound`.

## 4. Table Mapping

By default, Blaze assumes the table name matches the struct name. You can override this using a static member:

```cpp
struct Product {
    static constexpr auto table_name = "shop_products";
    int id;
    // ...
};
```

## 5. Why use Repositories?
1.  **Typesafety:** Results are automatically mapped to C++ structs.
2.  **Safety:** All queries use parameterized inputs to prevent SQL Injection.
3.  **Clean Code:** Eliminates hundreds of lines of string-concatenated SQL boilerplate.
4.  **Zero Overhead:** Metadata is resolved at compile-time using reflection.
