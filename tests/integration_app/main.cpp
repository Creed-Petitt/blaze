#include <blaze/app.h>
#include <blaze/postgres.h>
#include <blaze/mysql.h>
#include <blaze/database.h>
#include <blaze/exceptions.h>
#include <blaze/model.h>
#include <blaze/client.h>
#include <blaze/middleware.h>
#include <blaze/wrappers.h>
#include <blaze/repository.h>
#include <blaze/crypto.h>
#include <iostream>
#include <vector>

using namespace blaze;

struct DataModel {
    int val;
};
BLAZE_MODEL(DataModel, val)

struct User {
    // Demo: Manual Table Name Override
    static constexpr const char* table_name = "User";
    
    int id;
    std::string name;
};
BLAZE_MODEL(User, id, name)

struct LoginRequest {
    std::string username;
    std::string password;

    void validate() const {
        if (username.empty()) throw BadRequest("Username required");
    }
};
BLAZE_MODEL(LoginRequest, username, password)

struct SearchRequest {
    std::string name;
};
BLAZE_MODEL(SearchRequest, name)

// NEW: Model without table_name override to test inference
struct AuditLog {
    int id;
    std::string action;
};
BLAZE_MODEL(AuditLog, id, action)

class UserService {
public:
    BLAZE_DEPS(Database)
    UserService(std::shared_ptr<Database> db) : db_(db) {}

    Async<void> get_data(Response& res) {
        try {
            // Feature: Variadic Query
            auto result = co_await db_->query<DataModel>("SELECT 1 as val WHERE 1 = $1", 1);
            
            if (result.empty()) throw NotFound("No data");

            res.json({
                {"service", "UserService"},
                {"value_from_db", result[0].val} 
            });
        } catch (const std::exception& e) {
            throw InternalServerError(e.what());
        }
        co_return;
    }

private:
    std::shared_ptr<Database> db_;
};

int main() {
    App app;
    // Silence logs
    app.log_to("/dev/null"); 

    // Rate Limiting (10M reqs / 60s for Benchmarks)
    app.use(middleware::rate_limit(10000000, 60));

    // JWT Auth
    std::string secret = "integration-secret-key";
    app.use(middleware::jwt_auth(secret));

    std::cout << "--- REGISTERING ABSTRACT SERVICES ---" << std::endl;
    
    try {
        MySql::install(app, "mysql://root:blaze_secret@127.0.0.1:3306/blaze", 10);
        Postgres::install(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to connect to one or more databases: " << e.what() << std::endl;
    }

    app.provide<UserService>();

    // AUTO MIGRATION (Ensures test environment is ready) ---
    auto migrate = [](App& a) -> Async<void> {
        try {
            auto db = a.services().resolve<Database>();
            co_await db->query("CREATE TABLE IF NOT EXISTS \"User\" (id INT PRIMARY KEY, name TEXT)");
            std::cout << "[Migration] 'User' table ready." << std::endl;
        } catch (...) {
            std::cerr << "[Migration] Warning: Could not run migrations." << std::endl;
        }
    };
    app.spawn(migrate(app));

    std::cout << "--- DEFINING ROUTES ---" << std::endl;

    // Login (Generates JWT)
    app.post("/login", [secret](Body<LoginRequest> creds) -> Async<Json> {
        if (creds.username == "admin" && creds.password == "password") {
            std::string token = crypto::jwt_sign({{"id", 1}, {"role", "admin"}}, secret);
            co_return Json({{"token", token}});
        }
        throw Unauthorized("Invalid credentials");
    });

    // Protected Route (Requires Auth)
    app.get("/protected", [](Request& req) -> Async<Json> {
        if (!req.is_authenticated()) throw Unauthorized("Please login");
        
        // Pointer-free User Access
        co_return Json({
            {"message", "You are authenticated"},
            {"user", req.user()} 
        });
    });

    // Service Injection
    app.get("/abstract", [](Response& res, UserService& user) -> Async<void> {
        co_await user.get_data(res);
    });

    // Repository - Find All
    app.get("/users", [](Repository<User> users) -> Async<std::vector<User>> {
        co_return co_await users.all();
    });

    // Repository - Get Count
    app.get("/users/count", [](Repository<User> users) -> Async<Json> {
        int count = co_await users.count();
        co_return Json({{"total", count}});
    });

    // Repository - Find by ID
    app.get("/users/:id", [](Path<int> id, Repository<User> users) -> Async<User> {
        co_return co_await users.find(id);
    });

    // Repository - Create
    app.post("/users", [](Body<User> user, Repository<User> users) -> Async<void> {
        co_await users.save(user);
    });

    // Repository - Update
    app.put("/users/:id", [](Body<User> user, Repository<User> users) -> Async<void> {
        co_await users.update(user);
    });

    // Repository - Delete
    app.del("/users/:id", [](Path<int> id, Repository<User> users) -> Async<void> {
        co_await users.remove(id);
    });

    // Fluent Query Builder
    app.get("/search", [](Query<SearchRequest> req, Repository<User> users) -> Async<std::vector<User>> {
        co_return co_await users.query()
            .where("name", "=", req.name)
            .limit(10)
            .all();
    });

    app.post("/modern-api", [](Body<std::vector<int>> inputs) -> Async<Json> {
        std::vector<int> outputs;
        // Feature: Implicit iteration via inheritance
        for(int i : inputs) outputs.push_back(i * 10);
        co_return Json(outputs);
    });

    app.post("/all-in-one", [](Body<User> user) -> Async<User> {
        User u = user;
        u.name += " (Verified)";
        co_return u;
    });

    app.get("/health", [](Response& res) -> Async<void> {
        res.send("OK");
        co_return;
    });

    // NEW: Test Implicit JSON Conversion
    app.get("/modern-json", []() -> Async<Json> {
        co_return Json{{"status", "modern"}, {"version", 2}};
    });

    // NEW: Test Service Locator in Handler
    app.get("/locator", [](Request& req) -> Async<Json> {
        auto db = req.resolve<Database>();
        co_return Json{{"has_db", (bool)db}};
    });

    // NEW: Test inferred table name (AuditLog -> audit_log)
    app.get("/audit", [](Repository<AuditLog> repo) -> Async<Json> {
        co_return Json{{"table", repo.table_name()}};
    });

    std::cout << "Integration App running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
