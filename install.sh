#!/bin/bash

set(CMAKE_CXX_STANDARD 20)

set -e

echo "--- Blaze CLI Installer ---"

# Check for Go
if ! command -v go &> /dev/null; then
    echo "Error: Go is not installed. Please install Go (golang.org) to build the CLI."
    exit 1
fi

# 1. Handle Remote Install
if [ ! -d "cli" ]; then
    echo "[+] Remote install detected. Cloning Blaze repository..."
    TEMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/Creed-Petitt/blaze.git "$TEMP_DIR"
    cd "$TEMP_DIR"
    REMOTE_INSTALL=true
fi

# 2. Build
echo "[+] Building Blaze CLI..."
if [ -d "cli" ]; then
    cd cli
    go build -o ../blaze_bin
    cd ..
else
    echo "Error: 'cli' directory not found."
    exit 1
fi

# 3. Install
echo "[+] Installing binary to /usr/local/bin..."
if [ -w /usr/local/bin ]; then
    mv blaze_bin /usr/local/bin/blaze
else
    sudo mv blaze_bin /usr/local/bin/blaze
fi

# 4. Cleanup
if [ "$REMOTE_INSTALL" = true ]; then
    echo "[+] Cleaning up temporary files..."
    cd /tmp
    rm -rf "$TEMP_DIR"
fi

echo "[+] Installation Successful!"
echo "[!] Type 'blaze init myproject' to start."
echo ""
echo "Note: To build generated projects, ensure you have:"
echo "  - CMake 3.20+"
echo "  - G++ / Clang (C++20 support)"
echo "  - OpenSSL (libssl-dev)"
echo "  - PostgreSQL (libpq-dev) [Optional for Postgres driver]"