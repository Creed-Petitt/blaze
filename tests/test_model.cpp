#include <catch2/catch_test_macros.hpp>
#include <blaze/model.h>
#include <blaze/json.h>
#include <boost/json.hpp>

struct TestUser {
    int id;
    std::string name;
    bool active;
};
BLAZE_MODEL(TestUser, id, name, active)

TEST_CASE("Model: JSON Serialization", "[model]") {
    TestUser u{1, "Alice", true};
    
    // Test to_value (Serialization)
    boost::json::value jv = boost::json::value_from(u);
    CHECK(jv.as_object().at("id").as_int64() == 1);
    CHECK(jv.as_object().at("name").as_string() == "Alice");
    CHECK(jv.as_object().at("active").as_bool() == true);
}

TEST_CASE("Model: JSON Deserialization", "[model]") {
    std::string json_str = R"({"id": 2, "name": "Bob", "active": false})";
    boost::json::value jv = boost::json::parse(json_str);
    
    // Test value_to (Deserialization)
    TestUser u = boost::json::value_to<TestUser>(jv);
    
    CHECK(u.id == 2);
    CHECK(u.name == "Bob");
    CHECK(u.active == false);
}

TEST_CASE("Model: Partial JSON Deserialization", "[model]") {
    // Missing fields should stay default
    std::string json_str = R"({"id": 3})";
    boost::json::value jv = boost::json::parse(json_str);
    
    TestUser u = boost::json::value_to<TestUser>(jv);
    
    CHECK(u.id == 3);
    CHECK(u.name == ""); // Default
}
