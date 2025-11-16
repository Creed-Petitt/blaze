#include "framework/app.h"
#include "framework/middleware.h"
#include <string_view>

int main() {
    App app;

    // Global middleware
    app.use(middleware::cors());
    app.use(middleware::limit_body_size(1024 * 1024));
    app.use(middleware::static_files("./public"));

    // Root route - Health check
    app.get("/", [](Request& req, Response& res) {
        static constexpr std::string_view kHealthPayload =
            R"({"status":"ok","version":"1.0.0"})";
        res.json_raw(kHealthPayload);
    });

    // Redirect example - demonstrates res.redirect()
    app.get("/old-page", [](Request& req, Response& res) {
        res.redirect("/");
    });

    // API Routes Group - demonstrates route grouping

    auto api = app.group("/api");

    // Get all users with pagination - demonstrates req.get_query_int()
    api.get("/users", [](Request& req, Response& res) {
        int page = req.get_query_int("page", 1);
        int limit = req.get_query_int("limit", 10);
        std::string sort = req.get_query("sort", "name");

        res.json({
            {"users", {
                {{"id", 1}, {"name", "Alice"}, {"email", "alice@example.com"}},
                {{"id", 2}, {"name", "Bob"}, {"email", "bob@example.com"}},
                {{"id", 3}, {"name", "Charlie"}, {"email", "charlie@example.com"}}
            }},
            {"pagination", {
                {"page", page},
                {"limit", limit},
                {"sort", sort}
            }}
        });
    });

    // Get user by ID - demonstrates req.get_param_int() and res.not_found()
    api.get("/users/:id", [](Request& req, Response& res) {
        auto user_id = req.get_param_int("id");

        if (!user_id.has_value()) {
            res.bad_request("Invalid user ID - must be a number");
            return;
        }

        // Simulate user not found
        if (*user_id > 100) {
            res.not_found("User not found");
            return;
        }

        res.json({
            {"id", *user_id},
            {"name", "John Doe"},
            {"email", "john@example.com"},
            {"active", true}
        });
    });

    // Create user - demonstrates error handling and res.bad_request()
    api.post("/users", [](Request& req, Response& res) {
        try {
            auto data = req.json();

            // Validation - demonstrates res.bad_request()
            if (!data.contains("name") || !data.contains("email")) {
                res.bad_request("Missing required fields: name and email");
                return;
            }

            if (data["name"].get<std::string>().empty()) {
                res.bad_request("Name cannot be empty");
                return;
            }

            // Simulate successful creation
            res.status(201).json({
                {"message", "User created successfully"},
                {"user", {
                    {"id", 123},
                    {"name", data["name"]},
                    {"email", data["email"]}
                }}
            });

        } catch (const std::exception& e) {
            res.bad_request("Invalid JSON in request body");
        }
    });

    // Update user - demonstrates req.get_param_int() and validation
    api.put("/users/:id", [](Request& req, Response& res) {
        auto user_id = req.get_param_int("id");

        if (!user_id.has_value()) {
            res.bad_request("Invalid user ID");
            return;
        }

        try {
            auto data = req.json();

            res.json({
                {"message", "User updated successfully"},
                {"user", {
                    {"id", *user_id},
                    {"name", data.value("name", "Updated Name")},
                    {"email", data.value("email", "updated@example.com")}
                }}
            });

        } catch (const std::exception& e) {
            res.bad_request("Invalid JSON in request body");
        }
    });

    // Delete user - demonstrates res.no_content()
    api.del("/users/:id", [](Request& req, Response& res) {
        auto user_id = req.get_param_int("id");

        if (!user_id.has_value()) {
            res.bad_request("Invalid user ID");
            return;
        }

        // Simulate deletion
        res.no_content();
    });

    // Protected route - demonstrates req.get_header() and res.unauthorized()
    api.get("/protected", [](Request& req, Response& res) {
        std::string auth = req.get_header("Authorization");

        if (auth.empty()) {
            res.unauthorized("Authentication required");
            return;
        }

        if (auth != "Bearer secret-token") {
            res.forbidden("Invalid token or insufficient permissions");
            return;
        }

        res.json({
            {"message", "Access granted"},
            {"data", "Secret information"}
        });
    });


    // Admin Routes Group - demonstrates nested grouping

    auto admin = app.group("/admin");

    admin.get("/dashboard", [](Request& req, Response& res) {
        res.json({
            {"page", "dashboard"},
            {"stats", {
                {"users", 150},
                {"posts", 342},
                {"active_sessions", 23}
            }}
        });
    });

    admin.get("/settings", [](Request& req, Response& res) {
        res.json({
            {"page", "settings"},
            {"config", {
                {"maintenance_mode", false},
                {"debug", true}
            }}
        });
    });

    // API v2 - demonstrates nested grouping

    auto api_v2 = api.group("/v2");

    api_v2.get("/users", [](Request& req, Response& res) {
        res.json({
            {"version", "2.0"},
            {"users", {
                {{"id", 1}, {"name", "Alice V2"}}
            }}
        });
    });

    std::cout << "\n=== HTTP Server ===\n";
    std::cout << "Server starting on http://localhost:8080\n\n";
    std::cout << "Available endpoints:\n";
    std::cout << "  GET  /                     - Health check\n";
    std::cout << "  GET  /old-page             - Redirect example\n";
    std::cout << "  GET  /api/users            - List users (supports ?page=1&limit=10&sort=name)\n";
    std::cout << "  GET  /api/users/:id        - Get user by ID\n";
    std::cout << "  POST /api/users            - Create user (requires JSON body)\n";
    std::cout << "  PUT  /api/users/:id        - Update user (requires JSON body)\n";
    std::cout << "  DEL  /api/users/:id        - Delete user\n";
    std::cout << "  GET  /api/protected        - Protected route (requires Authorization header)\n";
    std::cout << "  GET  /admin/dashboard      - Admin dashboard\n";
    std::cout << "  GET  /admin/settings       - Admin settings\n";
    std::cout << "  GET  /api/v2/users         - API v2 users (nested grouping)\n";
    std::cout << "\nPress Ctrl+C to stop\n\n";

    app.listen(8080);

    return 0;
}
