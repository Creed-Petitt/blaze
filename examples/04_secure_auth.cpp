/**
 * Example 04: Secure Auth
 * 
 * This example demonstrates how to implement a secure Signup/Login flow.
 * Concepts:
 * - Password hashing with blaze::crypto
 * - JWT generation and verification
 * - JWT Middleware for protected routes
 * - Request-scoped user context
 * - Controller Pattern with Static Member Functions
 */

#include <blaze/app.h>
#include <blaze/middleware.h>
#include <blaze/crypto.h>
#include <blaze/repository.h>
#include <blaze/postgres.h>

using namespace blaze;

// 1. Define the User Model
struct Account {
    int id;
    std::string username;
    std::string password_hash;
};
BLAZE_MODEL(Account, id, username, password_hash)

// 2. Define Request Structs
struct AuthRequest {
    std::string username;
    std::string password;
};
BLAZE_MODEL(AuthRequest, username, password)

// Wrapper for the secret key so it can be injected via DI
struct SecretKey { std::string value; };

// 3. Controller Pattern
class AuthController {
public:
    static void register_routes(App& app) {
        app.post("/signup", &AuthController::signup);
        app.post("/login", &AuthController::login);
        app.get("/me", &AuthController::me);
    }

    static Async<Json> signup(Body<AuthRequest> req, Repository<Account> repo) {
        std::string hash = crypto::hash_password(req.password);
        Account acc{0, req.username, hash};
        co_await repo.save(acc);
        co_return Json({{"status", "account_created"}});
    }

    static Async<Json> login(Body<AuthRequest> req, Repository<Account> repo, SecretKey& secret) {
        auto users = co_await repo.query().where("username", "=", req.username).all();
        if (users.empty()) throw Unauthorized("Invalid username");

        if (!crypto::verify_password(req.password, users[0].password_hash)) {
            throw Unauthorized("Invalid password");
        }

        Json payload = {{"id", users[0].id}, {"username", users[0].username}};
        std::string token = crypto::jwt_sign(payload, secret.value, 3600);
        co_return Json({{"token", token}});
    }

    static Async<Json> me(Request& req) {
        if (!req.is_authenticated()) {
            throw Unauthorized("Please login first");
        }

        co_return Json({
            {"message", "Welcome back!"},
            {"user_data", req.user()}
        });
    }
};

int main() {
    App app;
    std::string secret_val = "blaze-tutorial-secret";
    
    // Provide the secret key to the DI container
    app.provide<SecretKey>(secret_val);

    // Setup DB
    try {
        auto db = Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10);
        app.service(db).as<Database>();
        app.spawn([](App& a) -> Async<void> {
            auto db = a.resolve<Database>();
            co_await db->query("CREATE TABLE IF NOT EXISTS accounts (id SERIAL PRIMARY KEY, username TEXT UNIQUE, password_hash TEXT)");
        }(app));
    } catch (...) {}

    // 4. Register JWT Middleware
    app.use(middleware::jwt_auth(secret_val));

    // 5. Register Controllers
    app.register_controllers<AuthController>();

    std::cout << "Auth API running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
