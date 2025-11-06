#include "../../framework/app.h"
#include "../../framework/middleware.h"
#include <vector>
#include <algorithm>

// Todo struct with automatic JSON serialization
struct Todo {
    int id;
    std::string title;
    bool completed;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Todo, id, title, completed)

// In-memory "database"
std::vector<Todo> todos = {
    {1, "Learn C++", false},
    {2, "Build REST API", true},
    {3, "Deploy to production", false}
};

int next_id = 4;

int main() {
    App app;

    // Enable CORS for frontend apps
    app.use(middleware::cors());

    // API routes group
    auto api = app.group("/api");

    // List all todos (with optional filtering)
    api.get("/todos", [](Request& req, Response& res) {
        std::string filter = req.get_query("status", "all");

        if (filter == "completed") {
            std::vector<Todo> completed;
            std::copy_if(todos.begin(), todos.end(), std::back_inserter(completed),
                [](const Todo& t) { return t.completed; });
            res.json(completed);
        } else if (filter == "active") {
            std::vector<Todo> active;
            std::copy_if(todos.begin(), todos.end(), std::back_inserter(active),
                [](const Todo& t) { return !t.completed; });
            res.json(active);
        } else {
            res.json(todos);
        }
    });

    // Get single todo by ID
    api.get("/todos/:id", [](Request& req, Response& res) {
        auto id = req.get_param_int("id");
        if (!id.has_value()) {
            res.bad_request("Invalid todo ID - must be a number");
            return;
        }

        auto it = std::find_if(todos.begin(), todos.end(),
            [&](const Todo& t) { return t.id == *id; });

        if (it == todos.end()) {
            res.not_found("Todo not found");
            return;
        }

        res.json(*it);
    });

    // Create new todo
    api.post("/todos", [](Request& req, Response& res) {
        try {
            auto data = req.json();

            // Validation
            if (!data.contains("title") || data["title"].get<std::string>().empty()) {
                res.bad_request("Title is required and cannot be empty");
                return;
            }

            // Create todo
            Todo new_todo = {
                next_id++,
                data["title"],
                data.value("completed", false)
            };

            todos.push_back(new_todo);
            res.status(201).json(new_todo);

        } catch (const std::exception& e) {
            res.bad_request("Invalid JSON in request body");
        }
    });

    // Update existing todo
    api.put("/todos/:id", [](Request& req, Response& res) {
        auto id = req.get_param_int("id");
        if (!id.has_value()) {
            res.bad_request("Invalid todo ID - must be a number");
            return;
        }

        auto it = std::find_if(todos.begin(), todos.end(),
            [&](const Todo& t) { return t.id == *id; });

        if (it == todos.end()) {
            res.not_found("Todo not found");
            return;
        }

        try {
            auto data = req.json();

            // Update fields
            if (data.contains("title")) {
                it->title = data["title"];
            }
            if (data.contains("completed")) {
                it->completed = data["completed"];
            }

            res.json(*it);

        } catch (const std::exception& e) {
            res.bad_request("Invalid JSON in request body");
        }
    });

    // Delete todo
    api.del("/todos/:id", [](Request& req, Response& res) {
        auto id = req.get_param_int("id");
        if (!id.has_value()) {
            res.bad_request("Invalid todo ID - must be a number");
            return;
        }

        auto it = std::find_if(todos.begin(), todos.end(),
            [&](const Todo& t) { return t.id == *id; });

        if (it == todos.end()) {
            res.not_found("Todo not found");
            return;
        }

        todos.erase(it);
        res.no_content();  // 204 No Content
    });

    std::cout << "\n=== Todo REST API ===\n";
    std::cout << "Server running on http://localhost:3000\n\n";
    std::cout << "Available endpoints:\n";
    std::cout << "  GET    /api/todos           - List all todos\n";
    std::cout << "  GET    /api/todos/:id       - Get single todo\n";
    std::cout << "  POST   /api/todos           - Create todo\n";
    std::cout << "  PUT    /api/todos/:id       - Update todo\n";
    std::cout << "  DELETE /api/todos/:id       - Delete todo\n\n";
    std::cout << "Try: curl http://localhost:3000/api/todos\n\n";

    app.listen(3000);
    return 0;
}
