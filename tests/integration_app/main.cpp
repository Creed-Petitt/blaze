#include <blaze/app.h>
#include <blaze/postgres.h>
#include <blaze/mysql.h>
#include <blaze/database.h>
#include <blaze/exceptions.h>
#include <blaze/model.h>
#include <blaze/client.h>
#include <blaze/middleware.h>
#include <iostream>
#include <vector>

using namespace blaze;

struct DataModel {
    int val;
};
BLAZE_MODEL(DataModel, val)

class UserService {
public:
    BLAZE_DEPS(Database)
    UserService(std::shared_ptr<Database> db) : db_(db) {}

    Task get_data(Response& res) {
        try {
            // New Clean Syntax: Returns std::vector<DataModel>
            auto result = co_await db_->query<DataModel>("SELECT 1 as val");
            
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

    std::cout << "--- REGISTERING ABSTRACT SERVICES ---" << std::endl;

    // Fluent Service Registration
    // In CI/Test environment, we assume these services are available.
    // If one fails, the app might crash on startup, which fails the test (Good).
    
    try {
        app.service(MySql::open(app, "mysql://root:blaze_secret@127.0.0.1:3306/blaze", 10))
           .as<Database>();

        app.service(Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10))
           .as<Database>(); // Postgres wins the 'Database' slot
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to connect to one or more databases: " << e.what() << std::endl;
        // We continue, but routes using them will fail
    }

    app.provide<UserService>();

    std::cout << "--- DEFINING ROUTES ---" << std::endl;

    // ROUTE 1: Service Injection
    app.get("/abstract", [](Response& res, UserService& user) -> Task {
        co_await user.get_data(res);
    });

    // ROUTE 2: Direct Interface Injection (Manual Mode)
    app.get("/direct", [](Response& res, Database& db) -> Task {
        auto result = co_await db.query("SELECT 999 as val");
        // Manual mode: result[0] works directly now (Value Wrapper)
        int val = result[0]["val"].as<int>();
        res.json({{"method", "Direct Interface Injection"}, {"val", val}});
    });

    // ROUTE 3: Safe Parameterized Query (Postgres syntax)
    app.get("/safe-query", [](Response& res, Database& db) -> Task {
        std::vector<std::string> params = {"Hello from Parameters!"};
        auto result = co_await db.query("SELECT $1::text as echo", params);
        
        if (result.empty()) {
            throw NotFound("No results found in safe-query");
        } 
        
        res.json({
            {"echo", result[0]["echo"].as<std::string>()}
        });
    });

    // ROUTE 4: MySQL Safe Parameterized Query
    app.get("/mysql-safe", [](Response& res, MySql& db) -> Task {
        std::vector<std::string> params = {"Hello from MySQL!"};
        auto result = co_await db.query("SELECT ? as echo", params);
        
        if (result.empty()) {
            throw NotFound("No results found in mysql-safe");
        }

        res.json({
            {"echo", result[0]["echo"].as<std::string>()}
        });
    });

    // ROUTE 5: Modern API Showcase (New Features)
    // Expects JSON Body: [1, 2, 3]
    app.post("/modern-api", [](Request& req, Response& res) -> Task {
        // 1. Typed Request Body (One-Liner)
        // If JSON is invalid or not an array of ints, this throws automatically -> 500 (or we can catch for 400)
        std::vector<int> inputs;
        try {
            inputs = req.json<std::vector<int>>();
        } catch (const std::exception&) {
            throw BadRequest("Invalid JSON body: Expected array of integers");
        }

        if (inputs.empty()) {
            throw BadRequest("Input array cannot be empty");
        }

        // 2. Logic
        std::vector<int> outputs;
        for(int i : inputs) outputs.push_back(i * 10);

        // 3. Fluid Headers
        res.headers({
            {"X-Powered-By", "Blaze V1"},
            {"X-Count", std::to_string(outputs.size())}
        });

        // 4. Typed Response (Vector -> JSON)
        res.json(outputs);
        
        co_return;
    });

    // ROUTE 6: Async HTTP Client (Fetch)
    app.get("/fetch-test", [](Response& res) -> Task {
        try {
            // Calling a real external HTTPS API
            auto response = co_await blaze::fetch("https://httpbin.org/get");
            
            res.json({
                {"remote_status", response.status},
                {"remote_data", response.body}
            });
        } catch (const std::exception& e) {
            throw InternalServerError(std::string("Fetch Failed: ") + e.what());
        }
    });

    // ROUTE 7: Static File Cache Test
    app.use(middleware::static_files("tests/integration_app/public"));

    app.get("/health", [](Response& res) -> Task {
        res.send("OK");
        co_return;
    });

    std::cout << "Integration App running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
