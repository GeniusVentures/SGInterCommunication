# ThirdParty Integration Guide

This document explains how SGInterCommunication integrates with the GeniusVentures thirdparty repository.

## Overview

SGInterCommunication uses the GeniusVentures thirdparty repository (https://github.com/GeniusVentures/thirdparty.git) as a submodule to provide consistent, pre-tested versions of all dependencies across the GeniusVentures ecosystem.

## Submodule Setup

The thirdparty repository is included as a git submodule:

```bash
git submodule add https://github.com/GeniusVentures/thirdparty.git thirdparty
git submodule update --init --recursive
```

## Available Libraries

The following libraries from the thirdparty repository are used by SGInterCommunication:

### Core Dependencies
- **GTest**: Unit testing framework
  - Location: `thirdparty/GTest/`
  - Used for: Unit tests and integration tests
  - Status: Optional (tests disabled if not found)

- **libp2p**: Peer-to-peer networking library
  - Location: `thirdparty/libp2p/`
  - Used for: P2P IPC backend
  - Status: Optional (libp2p backend disabled if not found)

### Optional Dependencies
- **grpc/protobuf**: Protocol buffer serialization
  - Location: `thirdparty/grpc/` (includes protobuf)
  - Used for: Message serialization in full build mode
  - Status: Optional (falls back to simple message format if not found)

## Build Configuration

The CMake configuration automatically detects which libraries are available in the thirdparty directory and configures the build accordingly:

### Full Build (with thirdparty dependencies)
```bash
cmake .. -DSGIPC_MINIMAL_BUILD=OFF
```

### Minimal Build (without external dependencies)
```bash
cmake .. -DSGIPC_MINIMAL_BUILD=ON
```

## Library Detection Logic

1. **GTest**: 
   - Looks in `thirdparty/GTest/lib/cmake/GTest`
   - Falls back to system package if not found
   - Disables tests if neither found

2. **Protobuf**:
   - Looks in `thirdparty/grpc/build/lib/cmake/protobuf`
   - Skips protobuf if not found (as requested)
   - Enables minimal build mode if missing

3. **libp2p**:
   - Looks for `thirdparty/libp2p/CMakeLists.txt`
   - Adds as subdirectory if found
   - Disables libp2p backend if missing

## Build Definitions

The following preprocessor definitions are set based on available features:

- `SGIPC_MINIMAL_BUILD`: When using simple message format
- `SGIPC_ENABLE_PROTOBUF`: When protobuf is available
- `SGIPC_ENABLE_LIBP2P`: When libp2p is available

## Platform-Specific Notes

### Windows
- Uses pre-built binaries from thirdparty releases
- Automatic library detection via CMake find_package

### Linux/macOS
- Can use either pre-built binaries or build from source
- Standard CMake configuration

### iOS/Android
- Uses platform-specific builds from thirdparty
- Cross-compilation support

## Updating Dependencies

To update to newer versions of the thirdparty libraries:

```bash
cd thirdparty
git pull origin master
cd ..
git add thirdparty
git commit -m "Update thirdparty to latest version"
```

## Troubleshooting

### Library Not Found
If a library is not found:
1. Ensure the thirdparty submodule is properly initialized
2. Check that the expected directory structure exists
3. Verify the build directory contains the necessary CMake files

### Build Failures
- Use minimal build mode to bypass missing dependencies
- Check that the correct platform-specific build directory is used
- Ensure all required tools (CMake, compiler) are available

## Integration with GeniusVentures Projects

This thirdparty integration follows the same pattern used by other GeniusVentures projects:
- Consistent library versions across projects
- Platform-specific pre-built binaries
- Optional dependency resolution
- Graceful fallbacks when libraries are missing

For more information about the thirdparty repository, see:
https://github.com/GeniusVentures/thirdparty/blob/master/README.md
