#!/bin/bash
# llmzip deployment script for Linux/VPS

set -e

echo "--- Pulling latest changes from GitHub ---"
git pull origin main

echo "--- Preparing build directory ---"
mkdir -p build
cd build

echo "--- Running CMake ---"
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "--- Building llmzip ---"
make -j$(nproc)

echo "--- Testing build ---"
./llmzip version

echo "--- Done! llmzip is ready in build/ directory ---"
