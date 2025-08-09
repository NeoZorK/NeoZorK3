#!/bin/bash

# NeoZorK3 Build Script
# This script builds the NeoZorK3 project with local dependencies

set -e  # Exit on any error

echo "Building NeoZorK3..."

# Check if we're in the project root
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building the project..."
make -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build completed successfully!"
echo "Executable: $(pwd)/neozork3_cli"

# Show help to verify it works
echo ""
echo "Testing executable..."
./neozork3_cli --help | head -20
