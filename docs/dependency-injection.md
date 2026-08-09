# Services

Blaze keeps application services explicit. `App` owns a small service registry for the lifetime of the server, and each `Request` can resolve from that registry.

```cpp
struct UserService {
    Async<Json> list();
};

int main() {
    App app;

    app.services().emplace<UserService>();

    app.get("/users", [](Request& req) -> Async<Json> {
        auto users = req.service<UserService>();
        co_return co_await users->list();
    });

    app.listen(8080);
}
```

Services are stored as `std::shared_ptr<T>` because the app owns them while requests borrow them concurrently. Register services during startup before `listen()`.

```cpp
auto users = std::make_shared<UserService>();
app.services().add<UserService>(users);
```

There is no auto-wiring, transient lifetime system, or constructor inference in core. If a service needs dependencies, construct it directly in your application code and register the finished object.
