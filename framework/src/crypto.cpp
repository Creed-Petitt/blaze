#include <blaze/crypto.h>
#include <blaze/util/string.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/crypto.h>
#include <iomanip>
#include <memory>
#include <string_view>
#include <charconv>

namespace blaze::crypto {

    // RAII Helpers for OpenSSL
    struct BioDeleter { void operator()(BIO* b) { BIO_free_all(b); } };
    struct KdfDeleter { void operator()(EVP_KDF* k) { EVP_KDF_free(k); } };
    struct KdfCtxDeleter { void operator()(EVP_KDF_CTX* c) { EVP_KDF_CTX_free(c); } };

    using BioPtr = std::unique_ptr<BIO, BioDeleter>;
    using KdfPtr = std::unique_ptr<EVP_KDF, KdfDeleter>;
    using KdfCtxPtr = std::unique_ptr<EVP_KDF_CTX, KdfCtxDeleter>;

    namespace {
        BioPtr create_base64_sink() {
            BioPtr b64(BIO_new(BIO_f_base64()));
            if (!b64) return nullptr;
            BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
            
            BIO* mem = BIO_new(BIO_s_mem());
            if (!mem) return nullptr; 
            
            BIO_push(b64.get(), mem);
            return b64;
        }

        BioPtr create_base64_source(std::string_view input) {
            BioPtr b64(BIO_new(BIO_f_base64()));
            if (!b64) return nullptr;
            BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);

            BIO* mem = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
            if (!mem) return nullptr;

            BIO_push(b64.get(), mem);
            return b64;
        }
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
        BioPtr chain = create_base64_sink();
        if (!chain) return "";

        if (BIO_write(chain.get(), input.data(), static_cast<int>(input.size())) <= 0) return "";
        BIO_flush(chain.get());

        BUF_MEM *bufferPtr;
        BIO_get_mem_ptr(chain.get(), &bufferPtr);

        return std::string(bufferPtr->data, bufferPtr->length);
    }

    std::string base64_decode(std::string_view input) {
        BioPtr chain = create_base64_source(input);
        if (!chain) return "";

        std::string res;
        res.resize(input.size());
        int decoded_size = BIO_read(chain.get(), res.data(), static_cast<int>(input.size()));
        res.resize(decoded_size > 0 ? decoded_size : 0);
        
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
        RAND_bytes((unsigned char*)res.data(), static_cast<int>(length));
        return util::hex_encode(res);
    }

    std::string jwt_sign(const Json& payload, std::string_view secret, int expires_in) {
        std::string header = R"({"alg":"HS256","typ":"JWT"})";
        
        boost::json::value full_payload = static_cast<boost::json::value>(payload);
        if (full_payload.is_object()) {
            full_payload.as_object()["exp"] = std::time(nullptr) + expires_in;
        }

        std::string encoded_header = base64url_encode(header);
        std::string encoded_payload = base64url_encode(boost::json::serialize(full_payload));
        
        std::string data = encoded_header + "." + encoded_payload;
        std::string signature = hmac_sha256(secret, data);
        
        return data + "." + base64url_encode(signature);
    }

    Json jwt_verify(std::string_view token, std::string_view secret, JwtError* error) {
        if (error) *error = JwtError::None;

        size_t first_dot = token.find('.');
        size_t last_dot = token.rfind('.');
        if(first_dot == std::string::npos || last_dot == std::string::npos || first_dot == last_dot) {
            if (error) *error = JwtError::Malformed;
            return blaze::Json();
        }

        std::string_view data = token.substr(0, last_dot);
        std::string_view sig_b64 = token.substr(last_dot + 1);
        
        std::string expected_sig = hmac_sha256(secret, data);
        std::string received_sig = base64url_decode(sig_b64);

        if (expected_sig.size() != received_sig.size() ||
            CRYPTO_memcmp(expected_sig.data(), received_sig.data(), expected_sig.size()) != 0) {
            if (error) *error = JwtError::InvalidSignature;
            return blaze::Json();
        }

        try {
            std::string payload_json = base64url_decode(token.substr(first_dot + 1, last_dot - first_dot - 1));
            auto payload = boost::json::parse(payload_json);
            
            if (payload.is_object() && payload.as_object().contains("exp")) {
                auto& exp_val = payload.at("exp");
                if (exp_val.is_number()) {
                    if (std::time(nullptr) > exp_val.to_number<std::int64_t>()) {
                        if (error) *error = JwtError::Expired;
                        return blaze::Json(); 
                    }
                }
            }
            
            return blaze::Json(payload);
        } catch (...) {
            if (error) *error = JwtError::Malformed;
            return blaze::Json();
        }
    }

    std::string hash_password(std::string_view password) {
        unsigned char salt[16];
        unsigned char out[32];
        RAND_bytes(salt, sizeof(salt));

        KdfPtr kdf(EVP_KDF_fetch(nullptr, "SCRYPT", nullptr));
        KdfCtxPtr kctx(EVP_KDF_CTX_new(kdf.get()));

        OSSL_PARAM params[6];
        uint64_t n = 16384; 
        uint32_t r = 8;     
        uint32_t p = 1;     

        params[0] = OSSL_PARAM_construct_octet_string("pass", (void*)password.data(), password.size());
        params[1] = OSSL_PARAM_construct_octet_string("salt", (void*)salt, sizeof(salt));
        params[2] = OSSL_PARAM_construct_uint64("n", &n);
        params[3] = OSSL_PARAM_construct_uint32("r", &r);
        params[4] = OSSL_PARAM_construct_uint32("p", &p);
        params[5] = OSSL_PARAM_construct_end();

        if (EVP_KDF_derive(kctx.get(), out, sizeof(out), params) <= 0) {
            return "";
        }

        return "$s1$" + std::to_string(n) + "$" + std::to_string(r) + "$" + std::to_string(p) + "$" + 
               base64url_encode(std::string_view((char*)salt, 16)) + "$" + 
               base64url_encode(std::string_view((char*)out, 32));
    }

    bool verify_password(std::string_view password, std::string_view hash) {
        if (!hash.starts_with("$s1$")) return false;

        auto next_token = [&](size_t& pos) -> std::string_view {
            if (pos >= hash.size()) return {};
            size_t end = hash.find('$', pos);
            if (end == std::string_view::npos) {
                std::string_view res = hash.substr(pos);
                pos = hash.size();
                return res;
            }
            std::string_view res = hash.substr(pos, end - pos);
            pos = end + 1;
            return res;
        };

        size_t pos = 4; // Skip "$s1$"
        auto s_n = next_token(pos);
        auto s_r = next_token(pos);
        auto s_p = next_token(pos);
        auto s_salt = next_token(pos);
        auto s_hash = next_token(pos);

        if (s_n.empty() || s_r.empty() || s_p.empty() || s_salt.empty() || s_hash.empty()) return false;

        try {
            uint64_t n = 0;
            uint32_t r = 0;
            uint32_t p = 0;
            
            if (std::from_chars(s_n.data(), s_n.data() + s_n.size(), n).ec != std::errc()) return false;
            if (std::from_chars(s_r.data(), s_r.data() + s_r.size(), r).ec != std::errc()) return false;
            if (std::from_chars(s_p.data(), s_p.data() + s_p.size(), p).ec != std::errc()) return false;

            std::string salt = base64url_decode(s_salt);
            std::string expected_hash = base64url_decode(s_hash);

            unsigned char out[32];
            KdfPtr kdf(EVP_KDF_fetch(nullptr, "SCRYPT", nullptr));
            KdfCtxPtr kctx(EVP_KDF_CTX_new(kdf.get()));

            OSSL_PARAM params[6];
            params[0] = OSSL_PARAM_construct_octet_string("pass", (void*)password.data(), password.size());
            params[1] = OSSL_PARAM_construct_octet_string("salt", (void*)salt.data(), salt.size());
            params[2] = OSSL_PARAM_construct_uint64("n", &n);
            params[3] = OSSL_PARAM_construct_uint32("r", &r);
            params[4] = OSSL_PARAM_construct_uint32("p", &p);
            params[5] = OSSL_PARAM_construct_end();

            if (EVP_KDF_derive(kctx.get(), out, sizeof(out), params) <= 0) return false;

            if (expected_hash.size() != 32) return false;
            return CRYPTO_memcmp(out, expected_hash.data(), 32) == 0;
        } catch (...) {
            return false;
        }
    }

} // namespace blaze::crypto
