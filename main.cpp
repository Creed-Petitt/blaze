#include "framework/app.h"

int main() {
    App app;

    // Health check
    app.get("/", [](Request& req, Response& res) {
        res.send("OK\n");
    });

    // Get user by ID (demonstrates dynamic params + JSON response)
    app.get("/api/users/:id", [](Request& req, Response& res) {
        res.json({
            {"id", req.params["id"]},
            {"name", "John Doe"},
            {"email", "john@example.com"},
            {"active", true}
        });
    });

    // Create user (demonstrates JSON request parsing + nested response)
    app.post("/api/users", [](Request& req, Response& res) {
        auto data = req.json();
        res.status(201).json({
            {"message", "User created"},
            {"user", {
                {"name", data["name"]},
                {"email", data["email"]},
                {"id", 123}
            }}
        });
    });

    app.listen(8080);

    return 0;
}