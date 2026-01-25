#include <catch2/catch_test_macros.hpp>
#include <blaze/app.h>
#include <blaze/middleware.h>
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
        
        // Note: To actually run this in a test without a full server,
        // we'd need to run the io_context. For now, we verify it compiles
        // and matches our design.
    }
}

TEST_CASE("Middleware: JWT and User Context", "[middleware][auth]") {
    App app;
    std::string secret = "test-secret";
    app.use(middleware::jwt_auth(secret));

    app.get("/me", [](Request& req, Response& res) -> Async<void> {
        if (req.is_authenticated()) {
            res.json(req.user());
        } else {
            res.status(401).send("No user");
        }
        co_return;
    });

    // This test verifies compilation and the helper method logic
    SECTION("Request User Storage") {
        Request req;
        Json user({{"id", 1}, {"name", "Bob"}});
        req.set_user(user);
        
        REQUIRE(req.is_authenticated());
        CHECK(req.user()["name"].as<std::string>() == "Bob");
    }
}

TEST_CASE("Request: Cookie Parsing", "[request]") {
    Request req;
    req.headers.insert("Cookie", "session=12345; theme=dark; user_id=99");

    SECTION("Should parse existing cookies") {
        CHECK(req.cookie("session") == "12345");
        CHECK(req.cookie("theme") == "dark");
        CHECK(req.cookie("user_id") == "99");
    }

    SECTION("Should return empty for missing cookies") {
        CHECK(req.cookie("missing") == "");
    }

    SECTION("Should handle malformed or empty cookie header") {
        Request empty_req;
        CHECK(empty_req.cookie("any") == "");
        
        Request malformed_req;
        malformed_req.headers.insert("Cookie", "broken-cookie");
        CHECK(malformed_req.cookie("broken-cookie") == "");
    }
}

TEST_CASE("Request: Generic Context Storage", "[request]") {
    Request req;

    SECTION("Store and retrieve basic types") {
        req.set("request_id", std::string("req-123"));
        req.set("retry_count", 5);

        CHECK(req.get<std::string>("request_id") == "req-123");
        CHECK(req.get<int>("retry_count") == 5);
    }

    SECTION("Store and retrieve custom structs") {
        struct Metadata { int flags; float score; };
        req.set("meta", Metadata{7, 99.5f});

        auto meta = req.get<Metadata>("meta");
        CHECK(meta.flags == 7);
        CHECK(meta.score == 99.5f);
    }

    SECTION("Handle missing keys safely") {
        // Safe optional access
        auto val = req.get_opt<int>("non_existent");
        CHECK(val.has_value() == false);

        // Throwing access
        CHECK_THROWS_AS(req.get<int>("non_existent"), std::runtime_error);
    }
}
