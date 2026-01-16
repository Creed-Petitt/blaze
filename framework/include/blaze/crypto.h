#ifndef BLAZE_CRYPTO_H
#define BLAZE_CRYPTO_H

#include <string>
#include <string_view>
#include <vector>
#include <boost/json.hpp>

namespace blaze::crypto {

    std::string sha256(std::string_view input);
    std::string hmac_sha256(std::string_view key, std::string_view data);

    std::string base64_encode(std::string_view input);
    std::string base64_decode(std::string_view input);
    
    std::string base64url_encode(std::string_view input);
    std::string base64url_decode(std::string_view input);

    std::string hex_encode(std::string_view input);
    std::string random_token(size_t length = 32);

    std::string jwt_sign(const boost::json::value& payload, std::string_view secret, int expires_in = 3600);
    boost::json::value jwt_verify(std::string_view token, std::string_view secret);

} // namespace blaze::crypto

#endif
