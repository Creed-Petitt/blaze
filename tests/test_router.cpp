#include <catch2/catch_test_macros.hpp>
#include <blaze/router.h>
#include <blaze/request.h>
#include <blaze/response.h>

using namespace blaze;

TEST_CASE("Router: Static Route Matching", "[router]") {
    Router router;

    router.add_route("GET", "/hello", [](Request& req, Response& res) -> Async<void> {
        co_return;
    });

    SECTION("Exact match should succeed") {
        auto match = router.match("GET", "/hello");
        REQUIRE(match.has_value());
    }

    SECTION("Wrong method should fail") {
        auto match = router.match("POST", "/hello");
        CHECK_FALSE(match.has_value());
    }

    SECTION("Wrong path should fail") {
        auto match = router.match("GET", "/world");
        CHECK_FALSE(match.has_value());
    }
}

TEST_CASE("Router: Parameter Extraction", "[router]") {
    Router router;

    router.add_route("GET", "/user/:id", [](Request& req, Response& res) -> Async<void> {
        co_return;
    });

    SECTION("Dynamic parameter should match") {
        auto match = router.match("GET", "/user/123");
        REQUIRE(match.has_value());
        CHECK(match->params.at("id") == "123");
    }

    SECTION("Trailing slash should not break matching (standardized behavior)") {
        auto match = router.match("GET", "/user/123/");
        CHECK(match.has_value());
    }

    SECTION("Path parameters should be URL-decoded") {
        router.add_route("GET", "/profile/:name", [](Request&, Response&) -> Async<void> { co_return; });
        auto match = router.match("GET", "/profile/Jane%20Doe");
        REQUIRE(match.has_value());
        CHECK(match->params.at("name") == "Jane Doe");
        CHECK(match->path_values[match->path_values.size()-1] == "Jane Doe");
    }
}