#include <blaze/app.h>
#include <blaze/postgres.h>
#include <blaze/mysql.h>
#include <blaze/database.h>
#include <blaze/exceptions.h>
#include <blaze/model.h>
#include <blaze/client.h>
#include <blaze/middleware.h>
#include <blaze/wrappers.h>
#include <blaze/crypto.h>
#include <iostream>
#include <vector>

using namespace blaze;

struct DataModel {
    int val;
};
BLAZE_MODEL(DataModel, val)

struct User {
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
    app.log_to("/dev/null");

    // Feature: Rate Limiting (1000 reqs / 60s)
    app.use(middleware::rate_limit(1000, 60));

    // Feature: JWT Auth
    std::string secret = "integration-secret-key";
    app.use(middleware::jwt_auth(secret));

    std::cout << "--- REGISTERING ABSTRACT SERVICES ---" << std::endl;
    
    try {
        app.service(MySql::open(app, "mysql://root:blaze_secret@127.0.0.1:3306/blaze", 10))
           .as<Database>();

        app.service(Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10))
           .as<Database>();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to connect to one or more databases: " << e.what() << std::endl;
    }

    app.provide<UserService>();

    std::cout << "--- DEFINING ROUTES ---" << std::endl;

    // Feature: Login (Generates JWT)
    app.post("/login", [secret](Body<LoginRequest> creds) -> Async<Json> {
        if (creds.username == "admin" && creds.password == "password") {
            std::string token = crypto::jwt_sign({{"id", 1}, {"role", "admin"}}, secret);
            co_return Json({{"token", token}});
        }
        throw Unauthorized("Invalid credentials");
    });

    // Feature: Protected Route (Requires Auth)
    app.get("/protected", [](Request& req) -> Async<Json> {
        if (!req.is_authenticated()) throw Unauthorized("Please login");
        
        // Feature: Pointer-free User Access
        co_return Json({
            {"message", "You are authenticated"},
            {"user", req.user()} 
        });
    });

    // Feature: Service Injection
    app.get("/abstract", [](Response& res, UserService& user) -> Async<void> {
        co_await user.get_data(res);
    });

    // Feature: Typed Path Injection + Variadic DB
    app.get("/users/:id", [](Path<int> id, Database& db) -> Async<User> {
        // "SELECT ... WHERE id = $1", 100
        // Feature: Implicit conversion for 'id'
        auto result = co_await db.query<User>("SELECT 42 as id, 'Alice' as name WHERE 1 = $1", id);
        if (result.empty()) throw NotFound("User not found");
        co_return result[0];
    });

    // Feature: Typed Body Injection
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

    std::cout << "Integration App running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
