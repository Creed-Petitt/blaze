# Testing & Security Guide

Blaze is designed to be a "Zero-Crash" framework. We employ industry-standard tools to verify memory safety, stability under load, and resistance to malicious attacks.

## 1. AddressSanitizer (ASan) & LeakSanitizer (LSan)

We use Google's AddressSanitizer to detect:
*   Use-After-Free (UAF)
*   Buffer Overflows
*   Memory Leaks
*   Stack Corruption

### Running a Sanitized Build
This build is 2-3x slower but strictly checks every memory access.

```bash
cd dev_test_dos/build
# Enable Sanitizers
cmake -DBLAZE_ENABLE_SANITIZERS=ON ..
make -j$(nproc)

# Run the Server
./dev_test_dos
```

If a memory error occurs, the server will crash immediately with a detailed stack trace.

## 2. Fuzz Testing

We use a custom Python fuzzer (`dev_test_dos/fuzz.py`) to bombard the server with malformed HTTP packets.

### The Attack Suite
*   **Huge Headers:** 10KB+ header fields.
*   **Binary Garbage:** Random byte streams.
*   **Slowloris:** Incomplete requests held open.
*   **Method Overflow:** Invalid HTTP methods (e.g., AAAA...GET).
*   **Negative Content-Length:** Integer overflow attacks.

### Running the Fuzzer
Ensure the server is running (preferably with ASan), then:

```bash
python3 dev_test_dos/fuzz.py
```

**Pass Criteria:** The server must accept/reject the request without crashing or leaking memory.

## 3. Endurance Testing (The 48-Hour Burn)

Before any release, we recommend a long-running load test to verify connection pooling and file descriptor management.

### Setup (DigitalOcean / AWS)
1.  Provision a standard Linux VM (2 vCPU, 4GB RAM).
2.  Install Blaze and build in `Release` mode.
3.  Start the server.

### The Attacker
Run this command from a separate machine (e.g., a cheap cloud VM) to bombard the server:

```bash
# -t2: 2 Threads
# -c50: 50 Concurrent Connections
# -d48h: Run for 48 hours
wrk -t2 -c50 -d48h -s db_test.lua http://<SERVER_IP>:8080/abstract
```

**Note:** Ensure `db_test.lua` is configured to hit a database-connected endpoint.

### Monitoring
Check the server's RAM usage periodically. It should remain flat.

```bash
# Monitor memory usage of the process
watch -n 1 "ps aux | grep dev_test_dos | grep -v grep"
```
