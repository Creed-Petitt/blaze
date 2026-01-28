/**
 * Example 02: Todo CRUD
 * 
 * This example demonstrates how to build a full CRUD API for a Todo list.
 * Concepts:
 * - Defining models with BLAZE_MODEL
 * - Using the Repository pattern for database access
 * - Path parameters with Path<T>
 * - Request body parsing with Body<T>
 * - JSON serialization
 */

#include <blaze/app.h>
#include <blaze/postgres.h> // Or blaze/mysql.h
#include <blaze/repository.h>

using namespace blaze;

// 1. Define the Data Model
struct Todo {
    int id;
    std::string title;
    bool completed;
};

// Enable reflection for the model
// This allows Blaze to map the struct to JSON and Database rows automatically
BLAZE_MODEL(Todo, id, title, completed)

int main() {
    App app;

    // 2. Setup Database (Requires a running PostgreSQL instance)
    // You can start one with: blaze docker psql
    try {
        auto db = Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10);
        app.service(db).as<Database>();

        // Run a quick migration to ensure the table exists
        app.spawn([](App& a) -> Async<void> {
            auto db = a.resolve<Database>();
            co_await db->query("CREATE TABLE IF NOT EXISTS todos (id SERIAL PRIMARY KEY, title TEXT, completed BOOLEAN)");
        }(app));
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }

    // 3. Define CRUD Routes

    // GET /todos - List all
    app.get("/todos", [](Repository<Todo> repo) -> Async<std::vector<Todo>> {
        // repo.all() returns a std::vector<Todo>
        co_return co_await repo.all();
    });

    // POST /todos - Create new
    app.post("/todos", [](Body<Todo> todo, Repository<Todo> repo) -> Async<void> {
        // Body<T> automatically parses the incoming JSON
        co_await repo.save(todo);
        co_return;
    });

    // GET /todos/:id - Get one
    app.get("/todos/:id", [](Path<int> id, Repository<Todo> repo) -> Async<Todo> {
        // Path<int> automatically extracts and converts the :id segment
        co_return co_await repo.find(id);
    });

    // PUT /todos/:id - Update
    app.put("/todos/:id", [](Body<Todo> todo, Repository<Todo> repo) -> Async<void> {
        co_await repo.update(todo);
        co_return;
    });

    // DELETE /todos/:id - Remove
    app.del("/todos/:id", [](Path<int> id, Repository<Todo> repo) -> Async<void> {
        co_await repo.remove(id);
        co_return;
    });

    std::cout << "Todo API running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
