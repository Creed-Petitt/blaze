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
    boost::json::value payload = {{"user_id", 123}, {"role", "admin"}};
    std::string secret = "super-secret-key";

    SECTION("Valid Token") {
        std::string token = jwt_sign(payload, secret, 3600);
        auto verified = jwt_verify(token, secret);
        
        REQUIRE(verified.is_object());
        CHECK(verified.as_object().at("user_id").as_int64() == 123);
    }

    SECTION("Invalid Secret") {
        std::string token = jwt_sign(payload, secret, 3600);
        auto verified = jwt_verify(token, "wrong-secret");
        CHECK(verified.is_null());
    }

    SECTION("Expired Token") {
        std::string token = jwt_sign(payload, secret, -10); // Expired 10s ago
        auto verified = jwt_verify(token, secret);
        CHECK(verified.is_null());
    }
}

TEST_CASE("Crypto: Password Hashing", "[crypto]") {
    std::string password = "password123";
    std::string hash = hash_password(password);

    CHECK(hash.find("$s1$") == 0);
    CHECK(verify_password(password, hash) == true);
    CHECK(verify_password("wrong-pass", hash) == false);
}
