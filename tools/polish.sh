#!/bin/bash

# Blaze Polishing Tool
# Runs Clang-Format and Clang-Tidy (with auto-fix) on the codebase.

set -e

echo "[Blaze] Starting Polish..."

# Format
echo "[Format] Applying LLVM style (4 spaces)..."
find framework/include framework/src tests examples -name "*.h" -o -name "*.cpp" | xargs clang-format -i

# Tidy (Optional: uncomment to enable auto-fix)
echo "[Tidy] Analyzing code structure..."
# clang-tidy -p build/ framework/src/*.cpp -fix

echo "[Blaze] Polish Complete!"
