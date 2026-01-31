#ifndef BLAZE_CRYPTO_H
#define BLAZE_CRYPTO_H

#include <string>
#include <string_view>
#include <vector>
#include <boost/json.hpp>
#include <blaze/json.h>

namespace blaze::crypto {

    std::string sha256(std::string_view input);
    std::string hmac_sha256(std::string_view key, std::string_view data);

    std::string base64_encode(std::string_view input);
    std::string base64_decode(std::string_view input);
    
    std::string base64url_encode(std::string_view input);
    std::string base64url_decode(std::string_view input);

    std::string hex_encode(std::string_view input);
    std::string random_token(size_t length = 32);

    enum class JwtError {
        None,
        Malformed,
        InvalidSignature,
        Expired
    };

    std::string jwt_sign(const Json& payload, std::string_view secret, int expires_in = 3600);
    Json jwt_verify(std::string_view token, std::string_view secret, JwtError* error = nullptr);

    std::string hash_password(std::string_view password);
    bool verify_password(std::string_view password, std::string_view hash);

} // namespace blaze::crypto

#endif
