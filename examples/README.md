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

# Run the file upload example
./examples/12_file_uploads
```
