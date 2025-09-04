# SGInterCommunication Architecture

## Overview

SGInterCommunication is a cross-platform IPC (Inter-Process Communication) service designed to enable communication between different instances of GeniusSDK applications running on the same device. The architecture follows a layered approach with multiple backend implementations to ensure maximum compatibility across platforms.

## Design Goals

1. **Cross-Platform Compatibility**: Support Android, iOS, Linux, Windows, and macOS
2. **Unified API**: Single interface regardless of underlying implementation
3. **Automatic Fallback**: Graceful degradation when primary backends are unavailable
4. **Discovery**: Automatic detection of other instances on the same device
5. **Minimal Footprint**: Lightweight integration with existing codebases
6. **Security**: Basic message validation and timestamp verification

## Architecture Layers

### 1. API Layer (Cross-Platform Interface)

The top layer provides a unified API through the `CrossPlatformIPC` abstract base class:

```cpp
class CrossPlatformIPC {
public:
    virtual IPCStatus init(const IPCConfig& config) = 0;
    virtual IPCStatus sendHeartbeat(uint16_t port) = 0;
    virtual IPCStatus negotiatePort(uint16_t preferred_port, uint16_t& assigned_port) = 0;
    virtual IPCStatus listenForMessages(MessageCallback callback) = 0;
    virtual IPCStatus sendMessage(const proto::IPCMessage& message, const std::string& recipient_id = "") = 0;
    // ... other methods
};
```

**Key Features:**
- Message serialization/deserialization using Protocol Buffers
- Timestamp validation for message freshness
- Instance ID generation and management
- Status code standardization

### 2. Backend Selection Layer

The factory function `createPlatformIPC()` selects the most appropriate backend for the current platform:

```cpp
std::unique_ptr<CrossPlatformIPC> createPlatformIPC() {
    // 1. Try libp2p (primary backend)
    if (libp2p_available) return std::make_unique<Libp2pIPC>();
    
    // 2. Try platform-specific fallbacks
    #ifdef iOS
    if (pasteboard_available) return std::make_unique<iOSPasteboardIPC>();
    #endif
    
    // 3. Universal fallback
    return std::make_unique<FileBasedIPC>();
}
```

### 3. Backend Implementation Layer

#### 3.1 libp2p Backend (Primary)

**Technology**: libp2p with GossipSub
**Platforms**: Linux, Windows, macOS, Android
**Advantages**: 
- Real-time communication
- Built-in peer discovery
- Scalable pubsub architecture
- Network-based, low latency

**Architecture**:
```
┌─────────────────┐    ┌─────────────────┐
│   Instance A    │    │   Instance B    │
│  (Libp2pIPC)   │    │  (Libp2pIPC)   │
└─────────┬───────┘    └─────────┬───────┘
          │                      │
          ├──── GossipSub ───────┤
          │    Topic: genius.ipc │
          │                      │
┌─────────▼───────┐    ┌─────────▼───────┐
│  libp2p Node A  │◄──►│  libp2p Node B  │
│  localhost:8080 │    │  localhost:8081 │
└─────────────────┘    └─────────────────┘
```

**Message Flow**:
1. Instances subscribe to `genius.ipc.local` topic
2. Heartbeats published periodically for discovery
3. Direct messaging via targeted pubsub messages
4. Automatic peer discovery through libp2p mechanisms

#### 3.2 iOS Pasteboard Backend (iOS Fallback)

**Technology**: UIPasteboard with named pasteboards
**Platform**: iOS (when libp2p unavailable)
**Advantages**:
- Works within iOS sandbox constraints
- No network permissions required
- Cross-app communication possible

**Limitations**:
- Polling-based (5-second intervals)
- Higher latency than network-based solutions
- Limited by iOS pasteboard restrictions

**Architecture**:
```
┌─────────────────┐    ┌─────────────────┐
│   App A         │    │   App B         │
│ (iOSPasteboard) │    │ (iOSPasteboard) │
└─────────┬───────┘    └─────────┬───────┘
          │                      │
          ▼                      ▼
    ┌──────────────────────────────────┐
    │       Named Pasteboards          │
    │  ┌─────────────────────────────┐ │
    │  │ com.genius.ipc.main         │ │
    │  │ com.genius.ipc.discovery    │ │
    │  │ com.genius.ipc.heartbeat    │ │
    │  └─────────────────────────────┘ │
    └──────────────────────────────────┘
```

**Message Flow**:
1. Messages serialized to JSON and written to named pasteboards
2. Polling thread checks pasteboards every 5 seconds
3. Message deduplication using unique IDs
4. Automatic cleanup of old messages

#### 3.3 File-Based Backend (Universal Fallback)

**Technology**: Temporary file system with directory watching
**Platforms**: All (universal fallback)
**Advantages**:
- Works on all platforms
- No special permissions required
- Reliable persistence

**Limitations**:
- Higher latency due to file I/O
- Requires file system access
- Polling-based detection

**Architecture**:
```
┌─────────────────┐    ┌─────────────────┐
│   Instance A    │    │   Instance B    │
│  (FileBasedIPC) │    │  (FileBasedIPC) │
└─────────┬───────┘    └─────────┬───────┘
          │                      │
          ▼                      ▼
    ┌──────────────────────────────────┐
    │     Temporary Directory          │
    │  /tmp/genius_ipc/               │
    │  ├── messages/                   │
    │  ├── heartbeat/                  │
    │  ├── discovery/                  │
    │  └── locks/                      │
    └──────────────────────────────────┘
```

**Message Flow**:
1. Messages serialized to binary files in specific directories
2. File watching thread scans directories every 2 seconds
3. File locking prevents concurrent access
4. Automatic cleanup of old files (10-minute TTL)

## Message Protocol

### Protocol Buffers Schema

```protobuf
message IPCMessage {
    MessageType type = 1;
    string sender_id = 2;
    string recipient_id = 3;
    uint64 timestamp = 4;
    
    oneof payload {
        Heartbeat heartbeat = 10;
        PortRequest port_request = 11;
        PortResponse port_response = 12;
    }
}

message Heartbeat {
    string instance_id = 1;
    uint32 port = 2;
    uint64 timestamp = 3;
    string version = 4;
    map<string, string> metadata = 5;
}
```

### Message Types

1. **Heartbeat**: Periodic presence announcements
2. **PortRequest**: Request for port assignment
3. **PortResponse**: Response to port request
4. **Custom Messages**: Extensible for future use

### Security Considerations

- **Timestamp Validation**: Messages older than TTL are rejected
- **Sender Verification**: Instance IDs prevent message spoofing
- **Message Freshness**: Configurable TTL prevents replay attacks
- **Future**: Encryption and digital signatures planned

## Threading Model

### libp2p Backend
```
Main Thread
├── Initialization & Configuration
├── Message Sending (blocking)
└── API Calls

Background Threads
├── Heartbeat Sender (periodic)
├── Peer Discovery (libp2p managed)
├── Message Reception (callback-based)
└── Peer Cleanup (periodic)
```

### File-Based Backend
```
Main Thread
├── Initialization & Configuration
├── File Writing (blocking)
└── API Calls

Background Threads
├── File Watching (periodic scanning)
├── Message Processing (callback invocation)
└── Cleanup Thread (old file removal)
```

### iOS Pasteboard Backend
```
Main Thread
├── Initialization & Configuration
├── Pasteboard Writing (blocking)
└── API Calls

Background Thread
├── Pasteboard Polling (5-second intervals)
├── Message Processing (callback invocation)
└── Cleanup (old message removal)
```

## Performance Characteristics

| Backend | Latency | Discovery Time | CPU Usage | Memory Usage |
|---------|---------|----------------|-----------|--------------|
| libp2p | ~10ms | ~1-2s | Medium | Medium |
| iOS Pasteboard | ~5s | ~10s | Low | Low |
| File-Based | ~2s | ~5s | Low | Low |

## Platform-Specific Considerations

### iOS
- **Sandbox Restrictions**: libp2p may not work in all scenarios
- **Background Execution**: Limited background processing time
- **Pasteboard Limitations**: iOS may clear pasteboards unpredictably
- **Entitlements**: Network access requires proper entitlements

### Android
- **Permissions**: File access and network permissions required
- **Battery Optimization**: Background threads may be limited
- **Storage Scoped**: File access restricted to app directories

### Windows
- **Firewall**: libp2p may require firewall exceptions
- **Temp Directory**: User-specific temporary directories
- **Service Integration**: Potential for Windows service integration

### Linux/macOS
- **Permissions**: Generally fewer restrictions
- **systemd/launchd**: Potential for system service integration
- **Network Stack**: Full libp2p functionality available

## Extension Points

### Custom Backends
Implement `CrossPlatformIPC` interface for custom backends:

```cpp
class CustomIPC : public CrossPlatformIPC {
    // Implement all virtual methods
};
```

### Custom Message Types
Extend protobuf schema for application-specific messages:

```protobuf
message CustomMessage {
    string custom_field = 1;
    // ... other fields
}

// Add to IPCMessage oneof payload
message IPCMessage {
    // ... existing fields
    oneof payload {
        // ... existing payloads
        CustomMessage custom_message = 20;
    }
}
```

### Discovery Mechanisms
Each backend can implement custom discovery:
- mDNS/Bonjour integration
- Bluetooth LE advertising
- QR code exchange
- NFC data exchange

## Future Enhancements

### Planned Features
1. **Message Encryption**: AES-256 encryption for sensitive data
2. **Digital Signatures**: Message authenticity verification
3. **Message Queuing**: Persistent message storage for offline scenarios
4. **Advanced Discovery**: mDNS, Bluetooth LE, NFC integration
5. **Performance Optimization**: Zero-copy message passing
6. **Monitoring**: Built-in metrics and health monitoring

### Scalability Considerations
- **Message Routing**: Support for message relaying between instances
- **Load Balancing**: Distribute messages across multiple backends
- **Federation**: Cross-device communication capabilities
- **Service Mesh**: Integration with service mesh architectures

## Integration with GeniusSDK

### GeniusNode Integration
```cpp
class GeniusNode {
private:
    std::shared_ptr<sgipc::CrossPlatformIPC> ipc_;
    
public:
    bool initializeIPC() {
        ipc_ = sgipc::createPlatformIPC();
        sgipc::IPCConfig config;
        config.instance_id = getNodeId();
        config.preferred_port = getPreferredPort();
        return ipc_->init(config) == sgipc::IPCStatus::SUCCESS;
    }
    
    void handleIPCMessage(const sgipc::proto::IPCMessage& message) {
        // Process incoming IPC messages
        // Coordinate with other GeniusNode instances
    }
};
```

### Port Coordination
```cpp
uint16_t GeniusNode::negotiateServicePort() {
    uint16_t assigned_port;
    auto status = ipc_->negotiatePort(preferred_port_, assigned_port);
    if (status == sgipc::IPCStatus::SUCCESS) {
        service_port_ = assigned_port;
        return assigned_port;
    }
    return 0;  // Failed to negotiate
}
```

This architecture provides a robust, extensible foundation for cross-platform IPC while maintaining simplicity and performance across diverse deployment scenarios.
