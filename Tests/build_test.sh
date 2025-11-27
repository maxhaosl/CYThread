#!/bin/bash

# Build script for CYThread test

set -e

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Building CYThread Test ==="
echo "Script directory: $SCRIPT_DIR"
echo "Root directory: $ROOT_DIR"

# Create build directory
BUILD_DIR="$SCRIPT_DIR/build"
mkdir -p "$BUILD_DIR"

# Create bin directory
BIN_DIR="$SCRIPT_DIR/bin"
mkdir -p "$BIN_DIR"

# Configure and build
cd "$BUILD_DIR"

echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "Building test..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build completed!"

# Check if the test executable was created
if [ -f "$BIN_DIR/CYThreadTest" ]; then
    echo "✅ Test executable created successfully: $BIN_DIR/CYThreadTest"
    
    # Make it executable
    chmod +x "$BIN_DIR/CYThreadTest"
    
    echo ""
    echo "To run the test:"
    echo "  cd $SCRIPT_DIR"
    echo "  ./bin/CYThreadTest"
else
    echo "❌ Test executable not found!"
    exit 1
fi
