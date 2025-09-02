#pragma once

#include "cross_platform_ipc.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#ifdef SGIPC_LIBP2P_AVAILABLE
// Forward declarations for libp2p types to avoid exposing headers
namespace libp2p::host { class Host; }
namespace libp2p::multi { class Multiaddress; }
namespace libp2p::peer { class PeerId; }
#endif

namespace sgipc
{

    /**
 * @brief libp2p-based IPC implementation for cross-platform communication
 * 
 * This implementation uses libp2p's GossipSub for localhost discovery and messaging.
 * It's the primary backend for most platforms and provides reliable peer-to-peer
 * communication with automatic discovery.
 */
    class Libp2pIPC : public CrossPlatformIPC
    {
    public:
        Libp2pIPC();
        ~Libp2pIPC() override;

        // CrossPlatformIPC interface implementation
        IPCStatus Init( const IPCConfig &config ) override;
        IPCStatus Shutdown() override;
        IPCStatus SendHeartbeat( uint16_t port ) override;
        IPCStatus NegotiatePort( uint16_t preferredPort, uint16_t &assignedPort ) override;
        IPCStatus ListenForMessages( MessageCallback callback ) override;
        IPCStatus StopListening() override;
        IPCStatus SendMessage( const proto::IPCMessage &message, const std::string &recipientId = "" ) override;
        std::vector<proto::DiscoveryAnnouncement> GetDiscoveredInstances() const override;
        bool                                      IsAvailable() const override;
        std::string                               GetBackendName() const override;
        const IPCConfig                          &GetConfig() const override;
        uint16_t                                  GetAssignedPort() const override;

    private:
        struct Peer
        {
            std::string                           peerId;
            std::string                           address;
            uint16_t                              port;
            std::chrono::steady_clock::time_point last_seen;
            proto::DiscoveryAnnouncement          announcement;
        };

        /**
     * @brief Initialize libp2p node
     * @return true on success, false on failure
     */
        bool InitializeLibp2pNode();

        /**
     * @brief Setup GossipSub topic for IPC communication
     * @return true on success, false on failure
     */
        bool SetupGossipSubTopic();

        /**
     * @brief Start discovery service for finding peers
     * @return true on success, false on failure
     */
        bool StartDiscoveryService();

        /**
     * @brief Handle incoming pubsub message
     * @param topic Topic name
     * @param data Message data
     * @param sender Sender peer ID
     */
        void HandlePubSubMessage( const std::string          &topic,
                                  const std::vector<uint8_t> &data,
                                  const std::string          &sender );

        /**
     * @brief Periodic heartbeat sender thread function
     */
        void HeartbeatSenderThread();

        /**
     * @brief Periodic peer cleanup thread function
     */
        void PeerCleanupThread();

        /**
     * @brief Find available port for listening
     * @param preferredPort Preferred port number
     * @return Available port number, 0 if none found
     */
        uint16_t FindAvailablePort( uint16_t preferredPort );

        /**
     * @brief Check if a port is available
     * @param port Port number to check
     * @return true if available, false otherwise
     */
        bool IsPortAvailable( uint16_t port );

        /**
     * @brief Update peer information
     * @param peerId Peer identifier
     * @param announcement Discovery announcement
     */
        void UpdatePeer( const std::string &peerId, const proto::DiscoveryAnnouncement &announcement );

        /**
     * @brief Remove stale peers that haven't been seen recently
     */
        void RemoveStalePeers();

        // Configuration and state
        IPCConfig             m_config;
        std::atomic<bool>     m_initialized{ false };
        std::atomic<bool>     m_listening{ false };
        std::atomic<bool>     m_shutdownRequested{ false };
        std::atomic<uint16_t> m_assignedPort{ 0 };

        // libp2p components
#ifdef SGIPC_LIBP2P_AVAILABLE
        // Basic implementation - no complex libp2p objects for now
        // These will be added when we implement full libp2p integration
#else
        // Use void* pointers when libp2p is not available (stub implementation)
        void *m_libp2pHost{ nullptr };
        void *m_pubsubService{ nullptr };
        void *m_discoveryService{ nullptr };
#endif

        // Threading
        std::thread     m_heartbeatThread;
        std::thread     m_cleanupThread;
        MessageCallback m_messageCallback;

        // Peer management
        mutable std::mutex                    m_peersMutex;
        std::unordered_map<std::string, Peer> m_discoveredPeers;

        // Topic name for IPC communication
        static constexpr const char *IPC_TOPIC       = "genius.ipc.local";
        static constexpr const char *DISCOVERY_TOPIC = "genius.discovery.local";

        // Cleanup intervals
        static constexpr auto PEER_TIMEOUT     = std::chrono::minutes( 5 );
        static constexpr auto CLEANUP_INTERVAL = std::chrono::minutes( 1 );
    };

} // namespace sgipc
