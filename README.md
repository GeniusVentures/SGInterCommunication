# SGInterCommunication

A cross-platform Inter-Process Communication (IPC) service for GeniusSDK applications. This library provides a unified API for inter-app communication across Android, iOS, Linux, Windows, and macOS platforms.

## Features

• **Cross-Platform Support**: Works on Android, iOS, Linux, Windows, and macOS
• **Multiple Backends**: libp2p for primary communication, with platform-specific fallbacks
• **Auto-Discovery**: Automatic discovery of other instances on the same device
• **Port Negotiation**: Automatic port conflict resolution
• **Message Types**: Support for heartbeat messages and custom IPC messages
• **Fallback Mechanisms**: Graceful fallback to alternative communication methods
• **Security**: Basic message validation and timestamp checking

## Architecture

The library uses a layered architecture with multiple backend implementations:

1. **Primary Backend**: libp2p with GossipSub for most platforms
2. **iOS Fallback**: UIPasteboard-based communication for iOS sandboxed apps
3. **File-Based Fallback**: Temporary file system communication for all platforms

See [Architecture.md](Architecture.md) for detailed design information.

## Dependencies

• **C++17**: Required for template features and modern C++ constructs
• **Protobuf**: For message serialization and cross-platform compatibility
• **libp2p**: Primary communication backend (when available)
• **CMake**: Build system (3.16 or higher)
• **Google Test**: For unit tests (optional, if BUILD_TESTS=ON)

## Project Structure

• `src/`: Source files for all IPC implementations
• `include/sgipc/`: Header files with public API
• `proto/`: Protobuf message definitions
• `test/`: Unit tests and integration tests
• `examples/`: Example applications demonstrating usage
• `build/`: Platform-specific build directories
• `cmake/`: CMake configuration files

## Building the Project

This project uses CMake with platform-specific build directories.

### Prerequisites

• Install CMake (3.16 or higher)
• Install a C++17-compatible compiler
• Install Protobuf development libraries
• Install Google Test (for testing)

### Build Instructions

For each platform, follow these steps. Example shown for Windows:

#### Debug Build
```cmd
cd build/Windows
mkdir Debug
cd Debug
cmake ../../.. -DCMAKE_BUILD_TYPE=Debug -G "Ninja"
ninja
```

#### Release Build
```cmd
cd build/Windows
mkdir Release
cd Release
cmake ../../.. -DCMAKE_BUILD_TYPE=Release -G "Ninja"
ninja
```

#### Release with Debug Info
```cmd
cd build/Windows
mkdir RelWithDebInfo
cd RelWithDebInfo
cmake ../../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo -G "Ninja" -DSANITIZE_ADDRESS=ON
ninja
```

Replace `Windows` with `Linux`, `OSX`, `Android`, or `iOS` as needed.

### Running Tests

After building, run the test executable:

```cmd
cd build/Windows/Debug
./sgipc_test
```

### Running Examples

```cmd
cd build/Windows/Debug
./simple_ipc_example
```

## Usage

### Basic Example

```cpp
#include "sgipc/cross_platform_ipc.hpp"

int main() {
    // Create IPC instance
    auto ipc = sgipc::createPlatformIPC();
    
    // Configure
    sgipc::IPCConfig config;
    config.instance_id = "my_app_instance";
    config.preferred_port = 8080;
    
    // Initialize
    auto status = ipc->init(config);
    if (status != sgipc::IPCStatus::SUCCESS) {
        return 1;
    }
    
    // Set up message callback
    auto callback = [](const sgipc::proto::IPCMessage& message, const std::string& sender) {
        std::cout << "Received message from: " << message.sender_id() << std::endl;
    };
    
    // Start listening
    ipc->listenForMessages(callback);
    
    // Send heartbeat
    ipc->sendHeartbeat(8080);
    
    // Run your application...
    
    // Cleanup
    ipc->shutdown();
    return 0;
}
```

### Port Negotiation Example

```cpp
// Negotiate a port
uint16_t assigned_port;
auto status = ipc->negotiatePort(8080, assigned_port);
if (status == sgipc::IPCStatus::SUCCESS) {
    std::cout << "Assigned port: " << assigned_port << std::endl;
}
```

### Custom Message Example

```cpp
// Create custom message
sgipc::proto::IPCMessage message;
message.set_type(sgipc::proto::MessageType::HEARTBEAT);
message.set_sender_id("my_instance");
message.set_recipient_id("target_instance");  // Or empty for broadcast

// Send message
auto status = ipc->sendMessage(message, "target_instance");
```

## Platform-Specific Notes

### iOS
- Uses UIPasteboard fallback when libp2p is not available
- Requires appropriate entitlements for network access
- Polling-based message detection (5-second intervals)

### Android
- Uses libp2p when available
- Falls back to file-based communication
- May require storage permissions

### Windows/Linux/macOS
- Primary libp2p backend preferred
- File-based fallback in temporary directories
- No special permissions required

## Configuration Options

```cpp
sgipc::IPCConfig config;
config.instance_id = "unique_id";              // Auto-generated if empty
config.preferred_port = 8080;                  // Preferred port (0 = auto)
config.timeout = std::chrono::milliseconds(5000);
config.heartbeat_interval = std::chrono::milliseconds(10000);
config.message_ttl = std::chrono::milliseconds(30000);
config.enable_encryption = false;              // Future feature
config.allow_fallback = true;                  // Allow backend fallback
```

## Error Handling

All operations return `sgipc::IPCStatus` values:

- `SUCCESS`: Operation completed successfully
- `FAILED_TO_INITIALIZE`: Initialization failed
- `FAILED_TO_SEND`: Message sending failed
- `TIMEOUT`: Operation timed out
- `INVALID_MESSAGE`: Malformed message
- `NETWORK_ERROR`: Network communication error
- `PERMISSION_DENIED`: Insufficient permissions
- `BACKEND_NOT_AVAILABLE`: No suitable backend available

Use `sgipc::toString(status)` to get human-readable error descriptions.

## Testing

The project includes comprehensive test suites:

- **Unit Tests**: Test individual components (`sgipc_test`)
- **Integration Tests**: Test multi-instance scenarios (`integration_test`)

Run tests after building to verify functionality on your platform.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with appropriate tests
4. Submit a pull request

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## About

SGInterCommunication is part of the GeniusVentures ecosystem, providing essential IPC capabilities for distributed applications and services.

### Known Limitations

- iOS Pasteboard fallback has polling overhead
- File-based fallback may be slower than network-based backends
- Port negotiation complexity varies by backend
- Message encryption not yet implemented

### Future Enhancements

- Message encryption and authentication
- Advanced discovery mechanisms
- Performance optimizations
- Additional platform-specific backends
- Message queuing and persistence