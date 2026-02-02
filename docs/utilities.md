# Core Utilities

Blaze includes a set of high-performance, zero-allocation utilities that you can use in your own application logic. These are the same tools the framework uses internally for routing and data processing.

## String Manipulation

Include `<blaze/util/string.h>` to access these functions.

### `url_decode`
Decodes a URL-encoded string (converting `%20` to spaces, `+` to spaces, etc.).

```cpp
// "Hello World"
std::string decoded = blaze::util::url_decode("Hello%20World");
```

### `hex_encode`
Converts binary data into a lowercase hex string. Optimized for high throughput.

```cpp
std::string data = "\x01\xFF";
std::string hex = blaze::util::hex_encode(data); // "01ff"
```

### `to_snake_case`
Converts PascalCase or camelCase identifiers to snake_case. Useful for dynamic DB mapping.

```cpp
// "user_account"
std::string db_name = blaze::util::to_snake_case("UserAccount");
```

### `convert_string<T>`
A high-speed, type-safe parser using C++17 `std::from_chars`. Throws `blaze::BadRequest` (400) on failure.

```cpp
try {
    int id = blaze::util::convert_string<int>("123");
    double price = blaze::util::convert_string<double>("19.99");
} catch (const blaze::BadRequest& e) {
    // Handle invalid format
}
```

### `jwt_sign` & `jwt_verify`
Convenient shortcuts for standard JSON Web Token operations.

```cpp
// Create a token
std::string token = blaze::jwt_sign({{"user_id", 123}}, "secret_key");

// Verify a token
try {
    Json payload = blaze::jwt_verify(token, "secret_key");
} catch (const blaze::crypto::JwtError& e) {
    // Handle invalid/expired token
}
```

---

## Circuit Breaker

Include `<blaze/util/circuit_breaker.h>` to use the generic circuit breaker pattern.

```cpp
// Threshold: 5 failures
// Cooldown: 10 seconds
blaze::CircuitBreaker breaker(5, 10);

if (breaker.allow_request()) {
    try {
        do_risky_operation();
        breaker.record_success();
    } catch (...) {
        breaker.record_failure();
    }
} else {
    // Fail fast
}
```
