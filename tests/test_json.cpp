#include <catch2/catch_test_macros.hpp>
#include <blaze/json.h>

using namespace blaze;

TEST_CASE("JSON: Basic Wrapping and Access", "[json]") {
    SECTION("Boost JSON Value Wrapping") {
        boost::json::value v = {
            {"foo", "bar"},
            {"baz", 42},
            {"arr", {1, 2, 3}}
        };
        Json j(v);

        CHECK(j["foo"].as<std::string>() == "bar");
        CHECK(j["baz"].as<int>() == 42);
        CHECK(j["arr"].size() == 3);
        CHECK(j["arr"][1].as<int>() == 2);
    }

    SECTION("Empty JSON") {
        Json j;
        CHECK_FALSE(j.is_ok());
        CHECK(j.empty());
    }
}

TEST_CASE("JSON: Type Conversion", "[json]") {
    boost::json::value v = {
        {"str", "123"},
        {"num", 456}
    };
    Json j(v);

    SECTION("String to Int conversion") {
        CHECK(j["str"].as<int>() == 123);
    }

    SECTION("Int to String conversion") {
        CHECK(j["num"].as<std::string>() == "456");
    }
}
