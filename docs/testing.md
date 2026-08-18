# Testing & Security Guide

Blaze takes testing and backend safety extremely seriously. We verify every commit through a rigorous automated pipeline called the **CI Gauntlet**, ensuring memory safety, high performance, and protocol stability.

## 1. Automated CI/CD

Every push to Blaze triggers an automated suite of tests in GitHub Actions. The pipeline consists of three parallel jobs:

### Job A: Unit Tests (Logic)
*   **Platforms:** Ubuntu (GCC), macOS (Clang).
*   **Verification:** Runs the **Catch2** suite to verify core logic (services, routing, JSON, middleware, environment).
*   **Target:** `blaze_tests`.

### Job B: Internal Fuzzing
*   **Logic Fuzzing:** Sends malformed JSON types and invalid URL encodings to verify `400 Bad Request` handling.

### Job C: Memory Safety (ASan Auditor)
*   **AddressSanitizer (ASan):** Compiles the framework with strict memory checking enabled.
*   **Leak Detection:** Runs the full integration suite and sends a `SIGINT` signal to trigger a formal memory leak report.
*   **UBSan:** Detects undefined behavior (integer overflows, null pointer dereferences).

### Job D: Concurrency Safety (TSan Auditor)
*   **ThreadSanitizer (TSan):** Compiles the framework with runtime data-race detection enabled.
*   **Race Detection:** Exercises the coroutine scheduler and shared server state under load.
*   **Deadlock Prevention:** Verifies that internal mutexes are correctly synchronized across multi-threaded execution contexts.

---

## 2. Manual Testing & Debugging

If you are developing locally, you can run these checks manually.


### Standard Build
Use this for the normal local test loop.

```bash
./build.sh test
```

### ASan/TSan Build
Use this to find hidden memory and threading bugs during development.

```bash
cmake -S . -B build/asan -DBLAZE_BUILD_TESTS=ON -DBLAZE_ENABLE_SANITIZERS=ON
cmake --build build/asan --parallel
ctest --test-dir build/asan --output-on-failure

cmake -S . -B build/tsan -DBLAZE_BUILD_TESTS=ON -DBLAZE_ENABLE_TSAN=ON
cmake --build build/tsan --parallel
ctest --test-dir build/tsan --output-on-failure
```


### Performance Stress Test
Use `wrk` or another load-testing tool against a running example or application:

```bash
# Simple GET benchmark
wrk -t4 -c100 -d10s http://localhost:8080/health
```

## 3. Security Philosophy
*   **Concurrency Resilience:** Blaze keeps shared server state small and synchronized, preventing crashes caused by high-concurrency race conditions.
*   **Non-Blocking Timeouts:** Blaze implements mandatory socket timeouts (default 30s) to prevent **Slowloris** attacks from exhausting file descriptors.
*   **Sanitized Headers:** Incoming headers are parsed via Boost.Beast with strict size limits (`max_body_size`) to prevent buffer overflows.
