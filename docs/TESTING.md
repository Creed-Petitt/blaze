# Testing & Security Guide

Blaze takes testing and backend safety extremely seriously. We verify every commit through a rigorous automated pipeline called the **CI Gauntlet**, ensuring memory safety, high performance, and protocol stability.

## 1. Automated CI/CD

Every push to Blaze triggers an automated suite of tests in GitHub Actions. The pipeline consists of three parallel jobs:

### Job A: Unit Tests (Logic)
*   **Platforms:** Ubuntu (GCC), macOS (Clang).
*   **Verification:** Runs the **Catch2** suite to verify core logic (DI, Routing, JSON, Crypto, Environment).
*   **Target:** `blaze_tests`.

### Job B: Integration & Fuzzing (Reliability)
*   **Database Integration:** Boots real **PostgreSQL** and **MySQL** instances in Docker.
*   **SSL Verification:** Generates self-signed certificates on-the-fly.
*   **Fuzz Attack:** Runs a Python-based protocol fuzzer to bombard the parser with malicious packets.
*   **Performance Benchmark:** Runs `wrk` load tests against all core routes to ensure no performance regressions.

### Job C: Memory Safety (ASan Auditor)
*   **AddressSanitizer (ASan):** Compiles the framework with strict memory checking enabled.
*   **Leak Detection:** Runs the full integration suite and sends a `SIGINT` signal to trigger a formal memory leak report.
*   **UBSan:** Detects undefined behavior (integer overflows, null pointer dereferences).

### Job D: Concurrency Safety (TSan Auditor)
*   **ThreadSanitizer (TSan):** Compiles the framework with runtime data-race detection enabled.
*   **Race Detection:** Exercises the coroutine scheduler and database connection pools under high load to ensure zero race conditions.
*   **Deadlock Prevention:** Verifies that internal mutexes and circuit breakers are correctly synchronized across multi-threaded execution contexts.

---

## 2. Manual Testing & Debugging

If you are developing locally, you can run these tools manually.


### ASan/TSan Build
Use this to find hidden memory and threading bugs during development.

```bash
mkdir build_sanitizers && cd build_sanitizers
cmake -DBLAZE_ENABLE_ASAN=ON .. 
cmake -DBLAZE_ENABLE_TSAN=ON ..
make -j$(nproc)
./tests/blaze_tests
```


### Manual Fuzzing
Ensure your server is running (with ASan for best results), then launch the attack:

```bash
python3 tests/integration_app/fuzz.py
```

### Performance Stress Test
We use `wrk` for high-concurrency benchmarks.

```bash
# Simple GET benchmark
wrk -t4 -c100 -d10s http://localhost:8080/health

# POST with JSON payload (using Lua script)
wrk -t4 -c100 -d10s -s tests/integration_app/post.lua http://localhost:8080/modern-api

# Auto-Injection Stress Test
wrk -t4 -c100 -d10s -s tests/integration_app/user_post.lua http://localhost:8080/all-in-one
```

## 3. Security Philosophy
*   **Concurrency Resilience:** Blaze guarantees that internal state (like Circuit Breakers and Connection Pools) is thread-safe, preventing crashes caused by high-concurrency race conditions.
*   **Non-Blocking Timeouts:** Blaze implements mandatory socket timeouts (default 30s) to prevent **Slowloris** attacks from exhausting file descriptors.
*   **Sanitized Headers:** Incoming headers are parsed via Boost.Beast with strict size limits (`max_body_size`) to prevent buffer overflows.
*   **Safe DB Access:** Our `Database` API uses parameterized queries by default, making SQL injection impossible.