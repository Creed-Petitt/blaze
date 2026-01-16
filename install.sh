#!/bin/bash

set -e

echo "--- Blaze CLI Installer ---"

# Check for Go
if ! command -v go &> /dev/null; then
    echo "Error: Go is not installed. Please install Go (golang.org) to build the CLI."
    exit 1
fi

# Build
echo "[+] Building Blaze CLI..."
if [ -d "cli" ]; then
    cd cli
    go build -o ../blaze_bin
    cd ..
else
    echo "Error: 'cli' directory not found. Run this script from the project root."
    exit 1
fi

# Install Binary
echo "[+] Installing binary to /usr/local/bin..."
if [ -w /usr/local/bin ]; then
    mv blaze_bin /usr/local/bin/blaze
else
    echo "    (Sudo required to write to /usr/local/bin)"
    sudo mv blaze_bin /usr/local/bin/blaze
fi

echo "[+] Installation Successful!"
echo "[!] Type 'blaze init myproject' to start."
echo ""
echo "Note: To build generated projects, ensure you have:"
echo "  - CMake 3.20+"
echo "  - G++ / Clang (C++20 support)"
echo "  - OpenSSL (libssl-dev)"
echo "  - PostgreSQL (libpq-dev)"
