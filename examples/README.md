# Blaze Showcase Suite

This directory contains 10 standalone, educational examples designed to teach you the Blaze framework from the ground up.

## List of Examples

1.  **01_hello_world**: The simplest starting point for any Blaze app.
2.  **02_todo_crud**: Full CRUD (Create, Read, Update, Delete) for a Task list using the Repository pattern.
3.  **03_realtime_chat**: A multi-client chatroom with automatic session cleanup and broadcasting.
4.  **04_secure_auth**: A full Login/Signup flow with hashed passwords and JWT token protection.
5.  **05_api_proxy**: Demonstrates how to use the non-blocking `blaze::fetch` client to call external APIs.
6.  **06_middleware_chain**: Advanced request timing and data passing via custom middleware.
7.  **07_advanced_queries**: Complex database filtering using the Fluent Query Builder.
8.  **08_production_ssl**: Running a secure server with HTTPS and production-grade logging.
9.  **09_background_tasks**: Using `app.spawn()` for non-blocking workers (Ghost coroutines).
10. **10_static_site**: Serving a frontend folder with high-performance RAM caching.

## How to Build

From the project root:

```bash
mkdir -p build && cd build
cmake .. -DBLAZE_BUILD_EXAMPLES=ON
make -j$(nproc)
```

## How to Run

After building, the executables will be in the `build/examples` directory.

```bash
# Run the Hello World example
./examples/01_hello_world

# Run the Todo CRUD example (Requires Postgres)
./examples/02_todo_crud
```

> **Note:** For examples that require a database (02, 04, 07), make sure you have a PostgreSQL instance running. You can start one easily using the Blaze CLI: `blaze docker psql`.
