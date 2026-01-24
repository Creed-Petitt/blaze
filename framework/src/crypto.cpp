#include <blaze/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/crypto.h>
#include <iomanip>
#include <sstream>

namespace blaze::crypto {

    std::string hex_encode(std::string_view input) {
        std::stringstream ss;
        for (unsigned char c : input) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return ss.str();
    }

    std::string sha256(std::string_view input) {
        unsigned char hash[32];
        unsigned int len = 0;
        EVP_Digest(input.data(), input.size(), hash, &len, EVP_sha256(), nullptr);
        return std::string((char*)hash, len);
    }

    std::string hmac_sha256(std::string_view key, std::string_view data) {
        unsigned char hash[32];
        unsigned int len = 32;
        HMAC(EVP_sha256(), key.data(), key.size(), (unsigned char*)data.data(), data.size(), hash, &len);
        return std::string((char*)hash, len);
    }

    std::string base64_encode(std::string_view input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, input.data(), input.size());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string res(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        return res;
    }

    std::string base64_decode(std::string_view input) {
        BIO *bio, *b64;
        std::string res;
        res.resize(input.size());

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_new_mem_buf(input.data(), input.size());
        bio = BIO_push(b64, bio);

        int decoded_size = BIO_read(bio, res.data(), input.size());
        res.resize(decoded_size > 0 ? decoded_size : 0);
        
        BIO_free_all(bio);
        return res;
    }

    std::string base64url_encode(std::string_view input) {
        std::string b64 = base64_encode(input);
        std::string res;
        res.reserve(b64.size());
        for(char c : b64) {
            if(c == '+') res += '-';
            else if(c == '/') res += '_';
            else if(c == '=') continue;
            else res += c;
        }
        return res;
    }

    std::string base64url_decode(std::string_view input) {
        std::string b64 = std::string(input);
        for(char& c : b64) {
            if(c == '-') c = '+';
            else if(c == '_') c = '/';
        }
        while(b64.size() % 4) b64 += '=';
        return base64_decode(b64);
    }

    std::string random_token(size_t length) {
        std::string res;
        res.resize(length);
        RAND_bytes((unsigned char*)res.data(), length);
        return hex_encode(res);
    }

    std::string jwt_sign(const boost::json::value& payload, std::string_view secret, int expires_in) {
        std::string header = R"({"alg":"HS256","typ":"JWT"})";
        
        boost::json::value full_payload = payload;
        if (full_payload.is_object()) {
            full_payload.as_object()["exp"] = std::time(nullptr) + expires_in;
        }

        std::string encoded_header = base64url_encode(header);
        std::string encoded_payload = base64url_encode(boost::json::serialize(full_payload));
        
        std::string data = encoded_header + "." + encoded_payload;
        std::string signature = hmac_sha256(secret, data);
        
        return data + "." + base64url_encode(signature);
    }

    boost::json::value jwt_verify(std::string_view token, std::string_view secret) {
        size_t first_dot = token.find('.');
        size_t last_dot = token.rfind('.');
        if(first_dot == std::string::npos || last_dot == std::string::npos || first_dot == last_dot) 
            return nullptr;

        std::string_view data = token.substr(0, last_dot);
        std::string_view sig_b64 = token.substr(last_dot + 1);
        
        std::string expected_sig = hmac_sha256(secret, data);
        std::string received_sig = base64url_decode(sig_b64);

        if (expected_sig.size() != received_sig.size() ||
            CRYPTO_memcmp(expected_sig.data(), received_sig.data(), expected_sig.size()) != 0) {
            return nullptr;
        }

        try {
            std::string payload_json = base64url_decode(token.substr(first_dot + 1, last_dot - first_dot - 1));
            auto payload = boost::json::parse(payload_json);
            
            if (payload.is_object() && payload.as_object().contains("exp")) {
                auto& exp_val = payload.at("exp");
                if (exp_val.is_number()) {
                    if (std::time(nullptr) > exp_val.to_number<std::int64_t>()) {
                        return nullptr; 
                    }
                }
            }
            
            return payload;
        } catch (...) {
            return nullptr;
        }
    }

    std::string hash_password(std::string_view password) {
        unsigned char salt[16];
        unsigned char out[32];
        RAND_bytes(salt, sizeof(salt));

        EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "SCRYPT", nullptr);
        EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);

        OSSL_PARAM params[6];
        uint64_t n = 16384; // CPU/Memory cost
        uint32_t r = 8;     // Block size
        uint32_t p = 1;     // Parallelization

        params[0] = OSSL_PARAM_construct_octet_string("pass", (void*)password.data(), password.size());
        params[1] = OSSL_PARAM_construct_octet_string("salt", (void*)salt, sizeof(salt));
        params[2] = OSSL_PARAM_construct_uint64("n", &n);
        params[3] = OSSL_PARAM_construct_uint32("r", &r);
        params[4] = OSSL_PARAM_construct_uint32("p", &p);
        params[5] = OSSL_PARAM_construct_end();

        if (EVP_KDF_derive(kctx, out, sizeof(out), params) <= 0) {
            EVP_KDF_CTX_free(kctx);
            EVP_KDF_free(kdf);
            return "";
        }

        EVP_KDF_CTX_free(kctx);
        EVP_KDF_free(kdf);

        // Encode as: $s1$N$r$p$salt_b64$hash_b64
        return "$s1$" + std::to_string(n) + "$" + std::to_string(r) + "$" + std::to_string(p) + "$" + 
               base64url_encode(std::string_view((char*)salt, 16)) + "$" + 
               base64url_encode(std::string_view((char*)out, 32));
    }

    bool verify_password(std::string_view password, std::string_view hash) {
        if (hash.substr(0, 4) != "$s1$") return false;

        // Parse format: $s1$N$r$p$salt$hash
        std::string s_hash(hash);
        std::vector<std::string> parts;
        size_t pos = 0;
        while ((pos = s_hash.find('$')) != std::string::npos) {
            parts.push_back(s_hash.substr(0, pos));
            s_hash.erase(0, pos + 1);
        }
        parts.push_back(s_hash);
        
        if (parts.size() < 7) return false;

        uint64_t n = std::stoull(parts[2]);
        uint32_t r = std::stoul(parts[3]);
        uint32_t p = std::stoul(parts[4]);
        std::string salt = base64url_decode(parts[5]);
        std::string expected_hash = base64url_decode(parts[6]);

        unsigned char out[32];
        EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "SCRYPT", nullptr);
        EVP_KDF_CTX *kctx = EVP_KDF_CTX_new(kdf);

        OSSL_PARAM params[6];
        params[0] = OSSL_PARAM_construct_octet_string("pass", (void*)password.data(), password.size());
        params[1] = OSSL_PARAM_construct_octet_string("salt", (void*)salt.data(), salt.size());
        params[2] = OSSL_PARAM_construct_uint64("n", &n);
        params[3] = OSSL_PARAM_construct_uint32("r", &r);
        params[4] = OSSL_PARAM_construct_uint32("p", &p);
        params[5] = OSSL_PARAM_construct_end();

        bool success = (EVP_KDF_derive(kctx, out, sizeof(out), params) > 0);
        
        EVP_KDF_CTX_free(kctx);
        EVP_KDF_free(kdf);

        if (!success) return false;

        if (expected_hash.size() != 32) return false;
        return CRYPTO_memcmp(out, expected_hash.data(), 32) == 0;
    }

} // namespace blaze::crypto