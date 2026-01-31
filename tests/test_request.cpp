#include <catch2/catch_test_macros.hpp>
#include <blaze/request.h>

using namespace blaze;

TEST_CASE("Request: URL Decoding", "[request]") {
    Request req;
    
    SECTION("Query parameters should be decoded") {
        // John+Doe -> John Doe (Space)
        req.set_target("/search?user=John+Doe");
        CHECK(req.query.at("user") == "John Doe");

        // John%2BDoe -> John+Doe (Literal Plus)
        req.set_target("/search?user=John%2BDoe");
        CHECK(req.query.at("user") == "John+Doe");
    }

    SECTION("Data isolation via clear()") {
        req.set_target("/test?a=1");
        CHECK(req.query.count("a") == 1);
        
        req.set_target("/test?b=2");
        CHECK(req.query.count("a") == 0);
        CHECK(req.query.at("b") == "2");
    }

    SECTION("Static url_decode helper") {
        CHECK(Request::url_decode("hello%20world") == "hello world");
        CHECK(Request::url_decode("a+b") == "a b");
        CHECK(Request::url_decode("%21%40%23") == "!@#");
        CHECK(Request::url_decode("incomplete%") == "incomplete%");
        CHECK(Request::url_decode("invalid%zz") == "invalid%zz");
    }
}

TEST_CASE("Request: Cookie Parsing Hardening", "[request]") {
    Request req;
    
    SECTION("Should handle spaces and quoted values") {
        req.headers.set(boost::beast::http::field::cookie, "session_id=\"abc 123\"; user_id=456; theme = dark ");
        
        CHECK(req.cookie("session_id") == "abc 123");
        CHECK(req.cookie("user_id") == "456");
        CHECK(req.cookie("theme") == "dark");
    }
}
