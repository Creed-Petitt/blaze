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

TEST_CASE("JSON: New Features (Arrays & Mutability)", "[json]") {
    SECTION("Array Construction (Clean Syntax)") {
        Json arr({1, 2, 3});
        REQUIRE(arr.size() == 3);
        CHECK(arr[0].as<int>() == 1);
        CHECK(arr[2].as<int>() == 3);
    }

    SECTION("Mixed Array Construction") {
        // Feature: Static helper for mixed types
        Json mixed = Json::array(1, "two", 3.5, true);
        
        CHECK(mixed.size() == 4);
        CHECK(mixed[0].as<int>() == 1);
        CHECK(mixed[1].as<std::string>() == "two");
        // Verify double conversion
        std::string dbl = mixed[2].as<std::string>();
        CHECK(dbl.starts_with("3.5")); 
        
        CHECK(mixed[3].as<std::string>() == "true");
    }

    SECTION("Object Construction (Clean Syntax)") {
        Json obj({{"name", "Blaze"}, {"id", 1}});
        CHECK(obj["name"].as<std::string>() == "Blaze");
        CHECK(obj["id"].as<int>() == 1);
    }

    SECTION("Mutability: set()") {
        Json obj({{"name", "Blaze"}});
        CHECK(obj["name"].as<std::string>() == "Blaze");

        obj.set("name", "Blaze V2");
        CHECK(obj["name"].as<std::string>() == "Blaze V2");

        obj.set("new_field", 100);
        CHECK(obj["new_field"].as<int>() == 100);
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

TEST_CASE("JSON: Error Handling", "[json]") {
    SECTION("try_get & has") {
        Json obj({{"name", "Blaze"}, {"id", 1}});
        
        CHECK(obj.has("name"));
        CHECK_FALSE(obj.has("missing"));

        // Valid Access
        auto name = obj.try_get<std::string>("name");
        CHECK(name.has_value());
        CHECK(name.value() == "Blaze");

        auto id = obj.try_get<int>("id");
        CHECK(id.has_value());
        CHECK(id.value() == 1);
        
        // Missing Key
        auto missing = obj.try_get<std::string>("missing");
        CHECK_FALSE(missing.has_value());
    }
}
