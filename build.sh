#!/bin/bash

# SGInterCommunication Build Script for Unix Systems

echo "SGInterCommunication Build Script"
echo "=================================="

# Function to detect platform
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "Linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "OSX"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "Windows"
    else
        echo "Unknown"
    fi
}

# Check if cmake is available
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found. Please install CMake."
    exit 1
fi

# Check if ninja is available
if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    BUILD_CMD="ninja"
else
    GENERATOR="Unix Makefiles"
    BUILD_CMD="make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
fi

# Set build configuration
BUILD_TYPE=${1:-Debug}
PLATFORM=$(detect_platform)

echo "Build Type: $BUILD_TYPE"
echo "Platform: $PLATFORM"
echo "Generator: $GENERATOR"

# Create build directory
mkdir -p "build/$PLATFORM/$BUILD_TYPE"

# Navigate to build directory
cd "build/$PLATFORM/$BUILD_TYPE"

# Configure with CMake
echo "Configuring project..."
cmake ../../.. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -G "$GENERATOR"
if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

# Build the project
echo "Building project..."
$BUILD_CMD
if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo "Build completed successfully!"
echo
echo "Executables are in: build/$PLATFORM/$BUILD_TYPE/"
echo
echo "To run tests: ./sgipc_test"
echo "To run examples: ./simple_ipc_example"
echo

cd ../../..
