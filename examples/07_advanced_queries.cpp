/**
 * Example 07: Advanced Queries
 * 
 * This example demonstrates how to use the Fluent Query Builder for complex filtering.
 * Concepts:
 * - repo.query() for complex filters
 * - .where(), .order_by(), .limit(), .offset()
 * - Automatic SQL Injection protection
 */

#include <blaze/app.h>
#include <blaze/repository.h>
#include <blaze/postgres.h>

using namespace blaze;

struct Product {
    int id;
    std::string name;
    double price;
    bool active;
};
BLAZE_MODEL(Product, id, name, price, active)

struct SearchParams {
    double min_price;
    bool active;
};
BLAZE_MODEL(SearchParams, min_price, active)

int main() {
    App app;

    // Setup DB
    try {
        auto db = Postgres::open(app, "postgresql://postgres:blaze_secret@127.0.0.1:5432/postgres", 10);
        app.service(db).as<Database>();
    } catch (...) {}

    // GET /products/search?min_price=10.0&active=true
    app.get("/products/search", [](Query<SearchParams> q, Repository<Product> repo) -> Async<std::vector<Product>> {
        // Start a fluent query
        auto builder = repo.query();

        // q is automatically parsed from the URL query string
        builder.where("price", ">=", q.min_price);
        builder.where("active", "=", q.active);

        // Return the top 10 expensive products matching the criteria
        co_return co_await builder
            .order_by("price", "DESC")
            .limit(10)
            .all();
    });

    std::cout << "Query demo running on :8080" << std::endl;
    app.listen(8080);

    return 0;
}
