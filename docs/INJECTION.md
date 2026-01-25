# Typed Injection & Validation

Blaze introduces a **Typed Injection** system that eliminates manual parsing and validation boilerplate. By declaring what you need in your handler arguments, the framework automatically extracts, converts, and validates the data before your code even runs.

## 1. Core Wrappers

Include `<blaze/wrappers.h>` to use these types.

### Path Parameters (`Path<T>`)
Maps to URL parameters (`:id`) based on order.

```cpp
// Route: /users/:id/files/:file_id
app.get("/users/:id/files/:file_id", [](Path<int> uid, Path<int> fid) -> Async<Json> {
    // Implicit conversion to int
    co_return Json({{"user", uid}, {"file", fid}});
});
```

### Request Body (`Body<T>`)
Automatically parses JSON into your `BLAZE_MODEL`. It inherits from your struct, so you can access members directly.

```cpp
struct User { int id; std::string name; };
BLAZE_MODEL(User, id, name)

app.post("/create", [](Body<User> user) -> Async<void> {
    // Direct access (no .value required)
    std::cout << "Creating user: " << user.name << std::endl;
});
```

### Query Parameters (`Query<T>`)
Maps the URL query string (`?q=hello&page=2`) to a model.

```cpp
struct Search { std::string q; int page; };
BLAZE_MODEL(Search, q, page)

app.get("/search", [](Query<Search> s) -> Async<Json> {
    co_return Json({{"results_for", s.q}, {"page", s.page}});
});
```

### Context Data (`Context<T>`)
Injects data stored by middleware via `req.set<T>()`.

```cpp
app.get("/log", [](Context<std::string> requestId) -> Async<void> {
    std::cout << "Processing request: " << requestId << std::endl;
});
```

---

## 2. Automatic Validation

Blaze automatically validates incoming data if you define a `validate()` method on your model. This works for `Body<T>`, `Query<T>`, and direct injection.

### How to use it
Just add `void validate() const` to your struct. Throw an exception if validation fails.

```cpp
struct Signup { 
    std::string email; 
    int age; 

    // Automatic Hook
    void validate() const {
        if (age < 18) throw BadRequest("Must be 18+");
        if (email.find('@') == std::string::npos) throw BadRequest("Invalid Email");
    }
};
BLAZE_MODEL(Signup, email, age)

app.post("/signup", [](Body<Signup> s) -> Async<void> {
    // If we get here, the data is GUARANTEED valid.
    save_user(s);
});
```

If validation fails, the client receives:
`400 Bad Request`
```json
{
    "error": "Bad Request",
    "message": "Must be 18+"
}
```

---

## 3. Advanced Features

### Implicit Conversion (The "Invisible" Wrapper)
Wrappers are designed to disappear. You rarely need to access `.value`.

*   **Structs:** `Body<User>` inherits from `User`. Use `user.id` directly.
*   **Containers:** `Body<vector<int>>` inherits from `vector`. Use `for(int i : body)` directly.
*   **Scalars:** `Path<int>` supports implicit conversion and math operators (`id + 1`).

### SFINAE & Performance
This system uses C++ templates (SFINAE) to resolve everything at **compile-time**. There is **zero runtime overhead** for using wrappers compared to manual parsing. The validation check is only generated if the `validate()` method exists.
