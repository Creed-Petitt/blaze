#include <catch2/catch_test_macros.hpp>
#include <blaze/response.h>

using namespace blaze;

TEST_CASE("Response: Beast-based Serialization", "[response]") {
    Response res;
    
    SECTION("Should use Beast for status and headers") {
        res.status(201)
           .header("X-Test", "Value")
           .send("Hello");
        
        std::string raw = res.build_response();
        
        // Basic check for presence of standard Beast-formatted response
        CHECK(raw.find("HTTP/1.1 201 Created") != std::string::npos);
        CHECK(raw.find("X-Test: Value") != std::string::npos);
        CHECK(raw.find("Content-Length: 5") != std::string::npos);
        CHECK(raw.find("\r\n\r\nHello") != std::string::npos);
    }

    SECTION("Should handle JSON") {
        res.json({{"status", "ok"}});
        std::string raw = res.build_response();
        
        CHECK(raw.find("application/json") != std::string::npos);
        CHECK(raw.find("{\"status\":\"ok\"}") != std::string::npos);
    }
}
