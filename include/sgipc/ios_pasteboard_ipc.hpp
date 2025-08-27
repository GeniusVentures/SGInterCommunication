#pragma once

#include "cross_platform_ipc.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_set>

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS

namespace sgipc
{

    /**
 * @brief iOS Pasteboard-based IPC implementation (fallback for iOS)
 * 
 * This implementation uses UIPasteboard for inter-app communication on iOS
 * when libp2p is not available or fails. It provides basic messaging
 * capabilities using named pasteboards and polling mechanisms.
 */
    class iOSPasteboardIPC : public CrossPlatformIPC
    {
    public:
        iOSPasteboardIPC();
        ~iOSPasteboardIPC() override;

        // CrossPlatformIPC interface implementation
        IPCStatus init( const IPCConfig &config ) override;
        IPCStatus shutdown() override;
        IPCStatus sendHeartbeat( uint16_t port ) override;
        IPCStatus negotiatePort( uint16_t preferredPort, uint16_t &assignedPort ) override;
        IPCStatus listenForMessages( MessageCallback callback ) override;
        IPCStatus stopListening() override;
        IPCStatus sendMessage( const proto::IPCMessage &message, const std::string &recipientId = "" ) override;
        std::vector<proto::DiscoveryAnnouncement> getDiscoveredInstances() const override;
        bool                                      isAvailable() const override;
        std::string                               getBackendName() const override;
        const IPCConfig                          &getConfig() const override;
        uint16_t                                  getAssignedPort() const override;

    private:
        struct PasteboardMessage
        {
            std::string                           message_id;
            std::vector<uint8_t>                  data;
            std::chrono::steady_clock::time_point timestamp;
            std::string                           senderId;
        };

        /**
     * @brief Initialize iOS pasteboard access
     * @return true on success, false on failure
     */
        bool initializePasteboard();

        /**
     * @brief Create or access named pasteboard for IPC
     * @param name Pasteboard name
     * @return Pasteboard handle (void* to avoid exposing iOS headers)
     */
        void *createNamedPasteboard( const std::string &name );

        /**
     * @brief Write message to pasteboard
     * @param pasteboard Pasteboard handle
     * @param message Serialized message data
     * @param message_id Unique message identifier
     * @return true on success, false on failure
     */
        bool writeToPasteboard( void *pasteboard, const std::vector<uint8_t> &message, const std::string &message_id );

        /**
     * @brief Read messages from pasteboard
     * @param pasteboard Pasteboard handle
     * @param messages Output vector of messages
     * @return true on success, false on failure
     */
        bool readFromPasteboard( void *pasteboard, std::vector<PasteboardMessage> &messages );

        /**
     * @brief Polling thread function for checking pasteboards
     */
        void pollingThread();

        /**
     * @brief Process received pasteboard messages
     * @param messages Vector of messages to process
     */
        void processMessages( const std::vector<PasteboardMessage> &messages );

        /**
     * @brief Generate unique message ID
     * @return Unique message identifier
     */
        std::string generateMessageId() const;

        /**
     * @brief Check if message has been seen before (deduplication)
     * @param message_id Message identifier
     * @return true if seen before, false if new
     */
        bool isMessageSeen( const std::string &message_id );

        /**
     * @brief Clean up old seen message IDs
     */
        void cleanupSeenMessages();

        /**
     * @brief Get JSON representation of discovery announcement
     * @param announcement Discovery announcement
     * @return JSON string
     */
        std::string serializeDiscoveryToJson( const proto::DiscoveryAnnouncement &announcement ) const;

        /**
     * @brief Parse JSON to discovery announcement
     * @param json JSON string
     * @param announcement Output announcement
     * @return true on success, false on failure
     */
        bool deserializeDiscoveryFromJson( const std::string &json, proto::DiscoveryAnnouncement &announcement ) const;

        // Configuration and state
        IPCConfig             m_config;
        std::atomic<bool>     m_initialized{ false };
        std::atomic<bool>     m_listening{ false };
        std::atomic<bool>     m_shutdownRequested{ false };
        std::atomic<uint16_t> m_assignedPort{ 0 };

        // iOS-specific handles
        void *m_mainPasteboard{ nullptr };      // Main IPC pasteboard
        void *m_discoveryPasteboard{ nullptr }; // Discovery pasteboard
        void *m_heartbeatPasteboard{ nullptr }; // Heartbeat pasteboard

        // Threading
        std::thread     m_pollingThread;
        MessageCallback m_messageCallback;

        // Message deduplication
        mutable std::mutex                    m_seenMessagesMutex;
        std::unordered_set<std::string>       m_seenMessageIds;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Discovered instances
        mutable std::mutex                        m_instancesMutex;
        std::vector<proto::DiscoveryAnnouncement> m_discoveredInstances;

        // Polling configuration
        static constexpr auto POLLING_INTERVAL         = std::chrono::milliseconds( 5000 );
        static constexpr auto MESSAGE_CLEANUP_INTERVAL = std::chrono::minutes( 10 );
        static constexpr auto messageTtl               = std::chrono::minutes( 5 );

        // Pasteboard names
        static constexpr const char *IPC_PASTEBOARD_NAME       = "com.geniusventures.ipc.main";
        static constexpr const char *m_discoveryPasteboardNAME = "com.geniusventures.ipc.discovery";
        static constexpr const char *m_heartbeatPasteboardNAME = "com.geniusventures.ipc.heartbeat";
    };

} // namespace sgipc

#endif // TARGET_OS_IOS
#endif // __APPLE__
