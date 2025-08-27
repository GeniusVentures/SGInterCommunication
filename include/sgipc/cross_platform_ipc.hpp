#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <cstdint>

#ifdef SGIPC_MINIMAL_BUILD
#include "sgipc/simple_messages.hpp"
namespace sgipc
{
    using MessageCallback =
        std::function<void( const simple::SimpleMessage &message, const std::string &senderAddress )>;
#else
#include "ipc_messages.pb.h"
namespace sgipc
{
    using MessageCallback = std::function<void( const proto::IPCMessage &message, const std::string &senderAddress )>;
#endif

    /**
 * @brief Status codes for IPC operations
 */
    enum class IPCStatus
    {
        SUCCESS = 0,
        FAILED_TO_INITIALIZE,
        FAILED_TO_SEND,
        FAILED_TO_RECEIVE,
        TIMEOUT,
        INVALID_MESSAGE,
        NETWORK_ERROR,
        PERMISSION_DENIED,
        BACKEND_NOT_AVAILABLE
    };

    /**
 * @brief Configuration for IPC backend
 */
    struct IPCConfig
    {
        std::string               instanceId;                 ///< Unique instance identifier
        uint16_t                  preferredPort = 0;          ///< Preferred port (0 = auto-assign)
        std::chrono::milliseconds timeout{ 5000 };            ///< Operation timeout
        std::chrono::milliseconds heartbeatInterval{ 10000 }; ///< Heartbeat send interval
        std::chrono::milliseconds messageTtl{ 30000 };        ///< Message time-to-live
        bool                      enableEncryption = false;   ///< Enable message encryption
        std::string               encryptionKey;              ///< Encryption key (if enabled)
        bool                      allowFallback = true;       ///< Allow fallback to alternative backends
    };

    /**
 * @brief Abstract base class for cross-platform IPC implementations
 * 
 * This class defines the unified API for Inter-Process Communication across
 * different platforms. Implementations provide platform-specific backends
 * (libp2p, Pasteboard, etc.) while maintaining a consistent interface.
 */
    class CrossPlatformIPC
    {
    public:
        virtual ~CrossPlatformIPC() = default;

        /**
     * @brief Initialize the IPC service
     * @param config Configuration parameters
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus Init( const IPCConfig &config ) = 0;

        /**
     * @brief Shutdown the IPC service and cleanup resources
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus Shutdown() = 0;

        /**
     * @brief Send a heartbeat message to announce presence
     * @param port The port this instance is listening on
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus SendHeartbeat( uint16_t port ) = 0;

        /**
     * @brief Negotiate a port with other instances
     * @param preferredPort Preferred port number
     * @param assignedPort Output parameter for the assigned port
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus NegotiatePort( uint16_t preferredPort, uint16_t &assignedPort ) = 0;

        /**
     * @brief Start listening for incoming messages
     * @param callback Function to call when messages are received
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus ListenForMessages( MessageCallback callback ) = 0;

        /**
     * @brief Stop listening for messages
     * @return IPCStatus indicating success or failure
     */
        virtual IPCStatus StopListening() = 0;

        /**
     * @brief Send a generic IPC message
     * @param message The message to send
     * @param recipientId Target instance ID (empty for broadcast)
     * @return IPCStatus indicating success or failure
     */
#ifdef SGIPC_MINIMAL_BUILD
        virtual IPCStatus SendMessage( const simple::SimpleMessage &message, const std::string &recipientId = "" ) = 0;
#else
        virtual IPCStatus SendMessage( const proto::IPCMessage &message, const std::string &recipientId = "" ) = 0;
#endif

        /**
     * @brief Get list of discovered instances
     * @return Vector of discovered service announcements
     */
#ifdef SGIPC_MINIMAL_BUILD
        virtual std::vector<simple::SimpleMessage> GetDiscoveredInstances() const = 0;
#else
        virtual std::vector<proto::DiscoveryAnnouncement> GetDiscoveredInstances() const = 0;
#endif

        /**
     * @brief Check if the backend is available on current platform
     * @return true if backend is available, false otherwise
     */
        virtual bool IsAvailable() const = 0;

        /**
     * @brief Get the name of the IPC backend
     * @return Backend name string
     */
        virtual std::string GetBackendName() const = 0;

        /**
     * @brief Get current configuration
     * @return Current IPCConfig
     */
        virtual const IPCConfig &GetConfig() const = 0;

        /**
     * @brief Get the currently assigned port
     * @return Assigned port number, 0 if not assigned
     */
        virtual uint16_t GetAssignedPort() const = 0;

    protected:
        /**
     * @brief Validate message timestamp for freshness
     * @param timestamp Message timestamp in milliseconds
     * @param ttl Time-to-live in milliseconds
     * @return true if message is fresh, false if stale
     */
        bool IsMessageFresh( uint64_t timestamp, std::chrono::milliseconds ttl ) const;

        /**
     * @brief Get current timestamp in milliseconds since epoch
     * @return Current timestamp
     */
        uint64_t GetCurrentTimestamp() const;

        /**
     * @brief Generate a unique instance ID
     * @return Unique instance identifier string
     */
        std::string GenerateInstanceId() const;

        /**
     * @brief Serialize IPC message to bytes
     * @param message Message to serialize
     * @param output Output byte vector
     * @return true on success, false on failure
     */
#ifdef SGIPC_MINIMAL_BUILD
        bool SerializeMessage( const simple::SimpleMessage &message, std::vector<uint8_t> &output ) const;
#else
        bool SerializeMessage( const proto::IPCMessage &message, std::vector<uint8_t> &output ) const;
#endif

        /**
     * @brief Deserialize bytes to IPC message
     * @param data Input byte data
     * @param message Output message
     * @return true on success, false on failure
     */
#ifdef SGIPC_MINIMAL_BUILD
        bool DeserializeMessage( const std::vector<uint8_t> &data, simple::SimpleMessage &message ) const;
#else
        bool DeserializeMessage( const std::vector<uint8_t> &data, proto::IPCMessage &message ) const;
#endif
    };

    /**
 * @brief Factory function to create appropriate IPC implementation for current platform
 * @return Shared pointer to IPC implementation
 */
    std::shared_ptr<CrossPlatformIPC> CreatePlatformIPC();

    /**
 * @brief Get human-readable string for IPC status
 * @param status IPCStatus value
 * @return Status description string
 */
    std::string ToString( IPCStatus status );

} // namespace sgipc
