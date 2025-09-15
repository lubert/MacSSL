#!/bin/bash

# Build script for MacSSL using Retro68 toolchain
# This replaces the CodeWarrior Pro 4 build process

echo "Building MacSSL with Retro68 toolchain..."

# Set up build directory
BUILD_DIR="build"

# Check for required RETRO68_BUILD_ROOT environment variable
if [ -z "$RETRO68_BUILD_ROOT" ]; then
    echo "Error: RETRO68_BUILD_ROOT environment variable is not set"
    echo "Please set it to your Retro68 build directory:"
    echo "Example: export RETRO68_BUILD_ROOT=/path/to/your/Retro68-build"
    exit 1
fi

TOOLCHAIN_PATH="$RETRO68_BUILD_ROOT/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake"

# Verify toolchain file exists
if [ ! -f "$TOOLCHAIN_PATH" ]; then
    echo "Error: Retro68 toolchain file not found at: $TOOLCHAIN_PATH"
    echo "Please verify RETRO68_BUILD_ROOT points to a valid Retro68 build directory"
    exit 1
fi

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake using Retro68 toolchain
echo "Configuring with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_PATH"

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building..."
make

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Output files:"
    ls -la *.dsk *.bin MacSSL* 2>/dev/null || echo "No output files found"
else
    echo "Build failed!"
    exit 1
fi