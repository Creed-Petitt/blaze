FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY CMakeLists.txt .
COPY main.cpp .
COPY thread_pool.h .
COPY json.hpp .
COPY framework/ ./framework/
COPY public/ ./public/

# Build the server
RUN mkdir build && cd build && \
    cmake .. && \
    make

# Expose HTTP port
EXPOSE 8080

WORKDIR /app/build
CMD ["./http_server"]
