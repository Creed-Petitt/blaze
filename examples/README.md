# Blaze Showcase Suite

This directory contains standalone, educational examples designed to teach you the Blaze framework from the ground up.

## List of Examples

1.  **01_hello_world**: The simplest starting point for any Blaze app.
2.  **03_realtime_chat**: A multi-client chatroom with automatic session cleanup and broadcasting.
3.  **06_middleware_chain**: Advanced request timing and data passing via custom middleware.
4.  **09_background_tasks**: Using `app.spawn()` for non-blocking workers.
5.  **10_static_site**: Serving a frontend folder with high-performance RAM caching.
6.  **12_file_uploads**: Handling multipart form uploads.

## How to Build

Each example is its own small CMake project. From the repository root:

```bash
cmake -S examples/01_hello_world -B build/examples/01_hello_world
cmake --build build/examples/01_hello_world --parallel
```

## How to Run

After building, run the executable from that example's build directory.

```bash
# Run the Hello World example
./build/examples/01_hello_world/01_hello_world

# Run the file upload example
cmake -S examples/12_file_uploads -B build/examples/12_file_uploads
cmake --build build/examples/12_file_uploads --parallel
./build/examples/12_file_uploads/12_file_uploads
```
