# SGInterCommunication Project Dependencies and Build Instructions

## Prerequisites

This project requires the following dependencies:

### Required Dependencies
- **CMake 3.16+** - Build system generator
- **C++20 compatible compiler** - MSVC 2019+, GCC 10+, or Clang 12+
- **Protobuf 3.21.0+** - Serialization library
- **Google Test** - Unit testing framework (optional)

### Platform-Specific Dependencies
- **Windows**: WinSock2 libraries (included with Windows SDK)
- **macOS/iOS**: Foundation and UIKit/AppKit frameworks
- **Linux**: Standard C++ libraries

## Installation Instructions

### Option 1: Using vcpkg (Recommended for Windows)

```powershell
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install protobuf gtest
.\vcpkg integrate install
```

### Option 2: Using Conan

```bash
# Install Conan if not already installed
pip install conan

# Install dependencies
conan install . --build=missing
```

### Option 3: System Package Managers

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install cmake build-essential
sudo apt-get install libprotobuf-dev protobuf-compiler
sudo apt-get install libgtest-dev
```

#### macOS with Homebrew
```bash
brew install cmake protobuf googletest
```

#### Windows with Chocolatey
```powershell
choco install cmake protobuf googletest
```

## Build Instructions

### Standard Build (with dependencies installed)

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Build with specific toolchain

```bash
# Using vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Using Conan
cmake .. -DCMAKE_PREFIX_PATH=path/to/conan/install/dir
```

### Build without tests (if GTest not available)

```bash
cmake .. -DBUILD_TESTS=OFF
cmake --build .
```

## Project Structure

```
SGInterCommunication/
├── cmake/                   # CMake configuration files (RLP-aligned)
│   ├── CommonBuildParameters.cmake
│   ├── CompilationFlags.cmake
│   ├── config.cmake.in
│   └── functions.cmake
├── include/sgipc/          # Public headers
│   ├── cross_platform_ipc.hpp
│   ├── libp2p_ipc.hpp
│   ├── file_ipc.hpp
│   └── ios_pasteboard_ipc.hpp
├── src/                    # Implementation files
│   ├── cross_platform_ipc.cpp
│   ├── libp2p_ipc.cpp
│   ├── file_ipc.cpp
│   └── ios_pasteboard_ipc.mm
├── proto/                  # Protobuf definitions
│   └── ipc_messages.proto
├── test/                   # Unit tests
│   ├── sgipc_test.cpp
│   └── integration_test.cpp
├── examples/               # Example applications
│   ├── simple_sender.cpp
│   ├── simple_receiver.cpp
│   └── discovery_demo.cpp
└── build/                  # Build output (generated)
```

## Configuration Options

The following CMake options are available:

- `BUILD_TESTS` (ON/OFF) - Build unit tests (requires GTest)
- `BUILD_EXAMPLES` (ON/OFF) - Build example applications
- `BUILD_SHARED_LIBS` (ON/OFF) - Build shared libraries instead of static
- `CMAKE_BUILD_TYPE` (Debug/Release/RelWithDebInfo/MinSizeRel) - Build configuration

## Cross-Platform Notes

### iOS Development
- Requires Xcode and iOS SDK
- Use `-DCMAKE_SYSTEM_NAME=iOS` for iOS builds
- Pasteboard backend is automatically enabled on iOS

### Android Development
- Requires Android NDK
- Use the Android NDK toolchain file
- File-based backend is used as primary communication method

### Windows Development
- Supports Visual Studio 2019+ and MinGW
- WinSock2 backend is automatically configured
- Can be built with both MSVC and Clang

## Troubleshooting

### Common Issues

1. **Protobuf not found**
   - Ensure Protobuf is installed and CMAKE_PREFIX_PATH includes the installation directory
   - On Windows, use vcpkg integration

2. **GTest not found**
   - Install Google Test or disable tests with `-DBUILD_TESTS=OFF`
   - Ensure GTest is in CMAKE_PREFIX_PATH

3. **C++20 not supported**
   - Update to a newer compiler version
   - GCC 10+, Clang 12+, or MSVC 2019+ required

4. **Platform-specific compilation errors**
   - Ensure platform-specific frameworks are available
   - Check that the correct SDK is installed

## Integration with GeniusSDK

This project follows the GeniusVentures project structure and can be integrated with other Genius SDK components:

- Uses the same CMake module structure as the RLP repository
- Compatible with the Genius build system
- Follows the same coding standards and conventions
- Can be used as a submodule in larger Genius projects

## License

This project follows the same license as other GeniusVentures components. See LICENSE file for details.
