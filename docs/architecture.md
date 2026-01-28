# Architecture & Design

Blaze is engineered for high-concurrency workloads where I/O latency is the primary bottleneck. This guide explores the design patterns and technical choices that make Blaze unique.

---

## 1. The Async Core

Blaze is built on top of **Boost.Asio** and C++20 **Coroutines**. Unlike traditional frameworks that use one thread per connection (which wastes memory and CPU), Blaze uses a small pool of worker threads to handle thousands of connections.

### How it works:
When a handler reaches a `co_await` point (like a database query), the current function is "frozen" and its state is saved. The worker thread is then immediately freed to handle a different request. Once the database returns the data, the original function is "thawed" and resumes exactly where it left off.

```mermaid
sequenceDiagram
    participant T as Worker Thread
    participant R1 as Request A (Database Query)
    participant R2 as Request B (Simple Static File)
    
    T->>R1: Start Handler A
    R1->>T: co_await Database (Suspend)
    Note over T: Thread is now free!
    T->>R2: Start Handler B
    R2->>T: Send File & co_return (Finish)
    Note over T: DB Result Ready
    T->>R1: Resume Handler A
    R1->>T: Send Result & co_return (Finish)
```

---

## 2. Dependency Injection Graph

Blaze's DI container is a **Scoped Service Provider**. It manages the lifecycle and delivery of objects across your application.

### The Service Hierarchy
1.  **Application Scope**: Singletons created once at startup (e.g., Database Pool, Global Config).
2.  **Request Scope**: Objects created or modified during a single HTTP request (e.g., The current User, a Trace ID).

```mermaid
graph LR
    subgraph AppContainer [Application Container]
        DB[(Database Pool)]
        Logger[Logger Service]
        Auth[Auth Service]
    end

    subgraph ReqContext [Request Context]
        User[Current User]
        Params[Path Params]
    end

    DB --> Repo[Repository]
    Repo --> Handler[Route Handler]
    User --> Handler
    Logger --> Handler
```

---

## 3. Reflection-Driven ORM

Blaze uses C++20 reflection (via `boost::describe`) to inspect your structs at compile-time. This is why you only need one line (`BLAZE_MODEL`) to get full JSON and Database support.

### The Metadata Pipeline:
1.  **Definition**: You define a struct with `BLAZE_MODEL`.
2.  **Inspection**: At compile-time, Blaze generates a list of fields and types.
3.  **Mapping**: 
    *   **JSON**: Blaze maps `struct.name` to `"name": "value"`.
    *   **SQL**: Blaze maps `struct.name` to `SELECT "name" FROM ...`.
4.  **Zero Overhead**: Because this happens at compile-time, there is no performance penalty compared to writing manual mapping code.

---

## 4. Security by Default

Blaze is designed with a "Secure-by-Default" philosophy:
*   **SQL Injection**: Every query in the `Repository` and `Database` classes is parameterized. You never concatenate strings to build SQL.
*   **Memory Safety**: Blaze uses RAII and smart pointers throughout. Raw `new` and `delete` are virtually non-existent in the codebase.
*   **Buffer Safety**: Powered by `boost::beast`, the HTTP parser handles malformed or oversized requests gracefully, preventing common overflow attacks.
