#include <catch2/catch_test_macros.hpp>
#include <blaze/crypto.h>

using namespace blaze::crypto;

TEST_CASE("Crypto: Base64 and Hex", "[crypto]") {
    std::string raw = "Blaze Framework";
    
    SECTION("Base64 Encoding/Decoding") {
        std::string encoded = base64_encode(raw);
        CHECK(encoded != raw);
        CHECK(base64_decode(encoded) == raw);
    }

    SECTION("Hex Encoding") {
        std::string hex = hex_encode("ABC");
        CHECK(hex == "414243");
    }
}

TEST_CASE("Crypto: JWT Signing and Verification", "[crypto]") {
    // New: Use blaze::Json
    blaze::Json payload = {{"user_id", 123}, {"role", "admin"}};
    std::string secret = "super-secret-key";

    SECTION("Valid Token") {
        std::string token = jwt_sign(payload, secret, 3600);
        auto verified = jwt_verify(token, secret);
        
        REQUIRE(verified.is_ok());
        CHECK(verified["user_id"].as<int>() == 123);
    }

    SECTION("Inline Syntax (Clean Look)") {
        // This tests explicit implicit conversion from initializer_list to Json
        std::string token = jwt_sign({
            {"id", 999},
            {"role", "superadmin"}
        }, secret);
        
        auto verified = jwt_verify(token, secret);
        CHECK(verified["id"].as<int>() == 999);
    }

    SECTION("Invalid Secret") {
        std::string token = jwt_sign(payload, secret, 3600);
        auto verified = jwt_verify(token, "wrong-secret");
        CHECK_FALSE(verified.is_ok());
    }

    SECTION("Expired Token") {
        std::string token = jwt_sign(payload, secret, -10); // Expired 10s ago
        auto verified = jwt_verify(token, secret);
        CHECK_FALSE(verified.is_ok());
    }
}

TEST_CASE("Crypto: Password Hashing", "[crypto]") {
    std::string password = "password123";
    std::string hash = hash_password(password);

    CHECK(hash.find("$s1$") == 0);
    CHECK(verify_password(password, hash) == true);
    CHECK(verify_password("wrong-pass", hash) == false);
}
