# Blaze Showcase Suite

This directory contains standalone, educational examples designed to teach you the Blaze framework from the ground up.

## List of Examples

1.  **01_hello_world**: The simplest starting point for any Blaze app.
2.  **02_todo_crud**: Full CRUD (Create, Read, Update, Delete) for a Task list using the Repository pattern.
3.  **03_realtime_chat**: A multi-client chatroom with automatic session cleanup and broadcasting.
4.  **05_api_proxy**: Demonstrates how to use the non-blocking `blaze::fetch` client to call HTTP APIs.
5.  **06_middleware_chain**: Advanced request timing and data passing via custom middleware.
6.  **07_advanced_queries**: Complex database filtering using the Fluent Query Builder.
7.  **09_background_tasks**: Using `app.spawn()` for non-blocking workers.
8.  **10_static_site**: Serving a frontend folder with high-performance RAM caching.
9.  **11_transactions**: Coordinating multi-step database writes.
10. **12_file_uploads**: Handling multipart form uploads.

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

> **Note:** For examples that require a database (02, 07, 11), make sure you have a PostgreSQL instance running. You can start one easily using the Blaze CLI: `blaze docker psql`.
