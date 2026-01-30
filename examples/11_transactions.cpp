/**
 * Example 11: Database Transactions
 * 
 * This example demonstrates how to perform atomic operations using raw SQL transactions.
 * Concepts:
 * - Raw Query execution
 * - BEGIN, COMMIT, ROLLBACK pattern
 * - Exception handling for atomicity
 */

#include <blaze/app.h>
#include <blaze/postgres.h>

using namespace blaze;

struct Account {
    int id;
    int balance;
};
BLAZE_MODEL(Account, id, balance)

int main() {
    App app;
    app.log_to("/dev/null");

    // Setup DB
    try {
        // Use a persistent DB for this example
        auto db = Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10);
        app.service(db).as<Database>();

        // Init Schema
        app.spawn([](App& a) -> Async<void> {
            try {
                auto db = a.resolve<Database>();
                co_await db->query("CREATE TABLE IF NOT EXISTS accounts (id SERIAL PRIMARY KEY, balance INT)");
                // Seed data
                co_await db->query("INSERT INTO accounts (id, balance) VALUES (1, 1000), (2, 1000) ON CONFLICT (id) DO NOTHING");
                std::cout << "Database initialized." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Init failed: " << e.what() << std::endl;
            }
        }(app));
    } catch (...) {}

    // POST /transfer
    // { "from": 1, "to": 2, "amount": 100 }
    app.post("/transfer", [](Body<Json> body, Database& db) -> Async<Json> {
        int from_id = body["from"].as<int>();
        int to_id = body["to"].as<int>();
        int amount = body["amount"].as<int>();

        // New Managed Transaction API
        // "tx_db" is a locked connection that lives only for this block.
        // Queries are automatically wrapped in BEGIN ... COMMIT/ROLLBACK
        co_await db.transaction([&](Database& tx_db) -> Async<void> {
            
            // 1. Debit
            auto res1 = co_await tx_db.query("UPDATE accounts SET balance = balance - $1 WHERE id = $2 RETURNING balance", amount, from_id);
            if (res1.empty()) throw BadRequest("Source account not found");
            if (res1[0]["balance"].as<int>() < 0) throw BadRequest("Insufficient funds");

            // 2. Credit
            auto res2 = co_await tx_db.query("UPDATE accounts SET balance = balance + $1 WHERE id = $2", amount, to_id);
            if (res2.affected_rows() == 0) throw BadRequest("Destination account not found");
            
            // 3. No explicit COMMIT needed! It happens automatically when this lambda returns.
            // If we throw (like above), it ROLLBACKs automatically.
        });
        
        co_return Json({{"status", "success"}, {"message", "Transfer complete"}});
    });

    // Advanced: Auto-Injection Transaction
    // Cleanest syntax: Inject Repositories directly!
    app.post("/transfer_clean", [](Body<Json> body, Database& db) -> Async<Json> {
        int from_id = body["from"].as<int>();
        int to_id = body["to"].as<int>();
        int amount = body["amount"].as<int>();

        co_await db.transaction([&](Repository<Account> accounts) -> Async<void> {
            // "accounts" is automatically created and bound to the transaction!
            
            // (Simulated logic using Repo for brevity)
            Account from = co_await accounts.find(from_id);
            if (from.balance < amount) throw BadRequest("Insufficient funds");
            
            from.balance -= amount;
            co_await accounts.update(from);

            Account to = co_await accounts.find(to_id);
            to.balance += amount;
            co_await accounts.update(to);
        });

        co_return Json({{"status", "success"}});
    });

    // Helper to see balances
    app.get("/accounts", [](Repository<Account> repo) -> Async<std::vector<Account>> {
        co_return co_await repo.all();
    });

    std::cout << "Transaction API running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
