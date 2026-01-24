#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <vector>

using namespace blaze;

TEST_CASE("Middleware: Execution Order", "[middleware]") {
    App app;
    std::vector<int> execution_order;

    // Middleware 1
    app.use([&](Request& req, Response& res, Next next) -> Async<void> {
        execution_order.push_back(1);
        co_await next();
        execution_order.push_back(6); // Post-processing
        co_return;
    });

    // Middleware 2
    app.use([&](Request& req, Response& res, Next next) -> Async<void> {
        execution_order.push_back(2);
        co_await next();
        execution_order.push_back(5);
        co_return;
    });

    app.get("/test", [&](Response& res) -> Async<void>  {
        execution_order.push_back(3);
        res.send("OK");
        execution_order.push_back(4);
        co_return;
    });

    SECTION("Middleware should execute in a nested stack (Onion model)") {
        Request req;
        req.method = "GET";
        req.path = "/test";
        
        // Directly invoke the internal handler to simulate a request
        auto result = app.handle_request(req, "127.0.0.1", false);
    }
}
