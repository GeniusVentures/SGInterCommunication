#include "sgipc/libp2p_ipc.hpp"
#include <iostream>
#include <algorithm>
#include <random>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment( lib, "ws2_32.lib" )
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace sgipc
{

    Libp2pIPC::Libp2pIPC() = default;

    Libp2pIPC::~Libp2pIPC()
    {
        Shutdown();
    }

    IPCStatus Libp2pIPC::Init( const IPCConfig &config )
    {
        if ( m_initialized.load() )
        {
            return IPCStatus::SUCCESS;
        }

        m_config = config;

        if ( m_config.instanceId.empty() )
        {
            m_config.instanceId = GenerateInstanceId();
        }

        // Initialize libp2p node
        if ( !InitializeLibp2pNode() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Setup GossipSub topic
        if ( !SetupGossipSubTopic() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Start discovery service
        if ( !startDiscoveryService() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Find and assign port
        uint16_t port = findAvailablePort( m_config.preferredPort );
        if ( port == 0 )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }
        m_assignedPort.store( port );

        m_initialized.store( true );
        m_shutdownRequested.store( false );

        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::shutdown()
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::SUCCESS;
        }

        m_shutdownRequested.store( true );
        m_listening.store( false );

        // Stop threads
        if ( m_heartbeatThread.joinable() )
        {
            m_heartbeatThread.join();
        }
        if ( m_cleanupThread.joinable() )
        {
            m_cleanupThread.join();
        }

        // Cleanup libp2p resources
        // TODO: Implement libp2p cleanup when integrating with actual libp2p library

        m_initialized.store( false );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::sendHeartbeat( uint16_t port )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        proto::IPCMessage message;
        message.set_type( proto::MessageType::HEARTBEAT );
        message.set_sender_id( m_config.instanceId );
        message.set_timestamp( getCurrentTimestamp() );

        auto heartbeat = message.mutable_heartbeat();
        heartbeat->set_instance_id( m_config.instanceId );
        heartbeat->set_port( port );
        heartbeat->set_timestamp( getCurrentTimestamp() );
        heartbeat->set_version( "1.0.0" );

        return sendMessage( message );
    }

    IPCStatus Libp2pIPC::negotiatePort( uint16_t preferredPort, uint16_t &assignedPort )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Simple port negotiation: check if preferred port is available
        if ( isPortAvailable( preferredPort ) )
        {
            assignedPort = preferredPort;
            m_assignedPort.store( preferredPort );
            return IPCStatus::SUCCESS;
        }

        // Find alternative port
        uint16_t alternative = findAvailablePort( preferredPort + 1 );
        if ( alternative == 0 )
        {
            return IPCStatus::FAILED_TO_SEND;
        }

        assignedPort = alternative;
        m_assignedPort.store( alternative );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::listenForMessages( MessageCallback callback )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        if ( m_listening.load() )
        {
            return IPCStatus::SUCCESS;
        }

        m_messageCallback = callback;
        m_listening.store( true );

        // Start background threads
        m_heartbeatThread = std::thread( &Libp2pIPC::heartbeatSenderThread, this );
        m_cleanupThread   = std::thread( &Libp2pIPC::peerCleanupThread, this );

        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::stopListening()
    {
        m_listening.store( false );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::sendMessage( const proto::IPCMessage &message, const std::string &recipientId )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Serialize message
        std::vector<uint8_t> serialized;
        if ( !serializeMessage( message, serialized ) )
        {
            return IPCStatus::INVALID_MESSAGE;
        }

        // TODO: Implement actual libp2p message sending
        // For now, simulate successful sending
        std::cout << "Sending message via libp2p (simulated)" << std::endl;

        return IPCStatus::SUCCESS;
    }

    std::vector<proto::DiscoveryAnnouncement> Libp2pIPC::getDiscoveredInstances() const
    {
        std::lock_guard<std::mutex>               lock( m_peersMutex );
        std::vector<proto::DiscoveryAnnouncement> instances;

        for ( const auto &[peerId, peer] : m_discoveredPeers )
        {
            instances.push_back( peer.announcement );
        }

        return instances;
    }

    bool Libp2pIPC::isAvailable() const
    {
        // TODO: Check if libp2p is actually available
        // For now, assume it's available on most platforms except iOS
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
        return false; // Prefer iOS-specific implementation
#endif
#endif
        return true;
    }

    std::string Libp2pIPC::getBackendName() const
    {
        return "libp2p";
    }

    const IPCConfig &Libp2pIPC::getConfig() const
    {
        return m_config;
    }

    uint16_t Libp2pIPC::getAssignedPort() const
    {
        return m_assignedPort.load();
    }

    // Private method implementations

    bool Libp2pIPC::initializeLibp2pNode()
    {
        // TODO: Initialize actual libp2p node
        // For now, simulate successful initialization
        std::cout << "Initializing libp2p node (simulated)" << std::endl;
        return true;
    }

    bool Libp2pIPC::setupGossipSubTopic()
    {
        // TODO: Setup actual GossipSub topic
        // For now, simulate successful setup
        std::cout << "Setting up GossipSub topic: " << IPC_TOPIC << std::endl;
        return true;
    }

    bool Libp2pIPC::startDiscoveryService()
    {
        // TODO: Start actual discovery service
        // For now, simulate successful start
        std::cout << "Starting discovery service (simulated)" << std::endl;
        return true;
    }

    void Libp2pIPC::handlePubSubMessage( const std::string          &topic,
                                         const std::vector<uint8_t> &data,
                                         const std::string          &sender )
    {
        if ( !m_listening.load() || !m_messageCallback )
        {
            return;
        }

        proto::IPCMessage message;
        if ( !deserializeMessage( data, message ) )
        {
            return;
        }

        // Validate message freshness
        if ( !isMessageFresh( message.timestamp(), m_config.messageTtl ) )
        {
            return;
        }

        // Update peer information if it's a heartbeat
        if ( message.type() == proto::MessageType::HEARTBEAT )
        {
            const auto                  &heartbeat = message.heartbeat();
            proto::DiscoveryAnnouncement announcement;
            announcement.set_instance_id( heartbeat.instanceId() );
            announcement.set_port( heartbeat.port() );
            announcement.set_timestamp( heartbeat.timestamp() );
            updatePeer( sender, announcement );
        }

        // Call user callback
        m_messageCallback( message, sender );
    }

    void Libp2pIPC::heartbeatSenderThread()
    {
        while ( !m_shutdownRequested.load() && m_listening.load() )
        {
            sendHeartbeat( m_assignedPort.load() );

            std::this_thread::sleep_for( m_config.heartbeatInterval );
        }
    }

    void Libp2pIPC::peerCleanupThread()
    {
        while ( !m_shutdownRequested.load() )
        {
            removeStalePeers();

            std::this_thread::sleep_for( CLEANUP_INTERVAL );
        }
    }

    uint16_t Libp2pIPC::findAvailablePort( uint16_t preferredPort )
    {
        // Start from preferred port and try a range
        for ( uint16_t port = preferredPort; port < preferredPort + 1000; ++port )
        {
            if ( port == 0 )
            {
                continue; // Skip port 0
            }
            if ( isPortAvailable( port ) )
            {
                return port;
            }
        }

        // Try random ports in the dynamic range
        std::random_device                      rd;
        std::mt19937                            gen( rd() );
        std::uniform_int_distribution<uint16_t> dis( 49152, 65535 );

        for ( int attempt = 0; attempt < 100; ++attempt )
        {
            uint16_t port = dis( gen );
            if ( isPortAvailable( port ) )
            {
                return port;
            }
        }

        return 0; // No available port found
    }

    bool Libp2pIPC::isPortAvailable( uint16_t port )
    {
#ifdef _WIN32
        SOCKET sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock == INVALID_SOCKET )
        {
            return false;
        }

        sockaddr_in addr;
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons( port );

        bool available = ( bind( sock, (sockaddr *)&addr, sizeof( addr ) ) != SOCKET_ERROR );
        closesocket( sock );
        return available;
#else
        int sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 )
        {
            return false;
        }

        sockaddr_in addr;
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons( port );

        bool available = ( bind( sock, (sockaddr *)&addr, sizeof( addr ) ) == 0 );
        close( sock );
        return available;
#endif
    }

    void Libp2pIPC::updatePeer( const std::string &peerId, const proto::DiscoveryAnnouncement &announcement )
    {
        std::lock_guard<std::mutex> lock( m_peersMutex );

        Peer &peer        = m_discoveredPeers[peerId];
        peer.peerId       = peerId;
        peer.port         = announcement.port();
        peer.last_seen    = std::chrono::steady_clock::now();
        peer.announcement = announcement;
    }

    void Libp2pIPC::removeStalePeers()
    {
        std::lock_guard<std::mutex> lock( m_peersMutex );

        auto now = std::chrono::steady_clock::now();
        auto it  = m_discoveredPeers.begin();

        while ( it != m_discoveredPeers.end() )
        {
            if ( now - it->second.last_seen > PEER_TIMEOUT )
            {
                it = m_discoveredPeers.erase( it );
            }
            else
            {
                ++it;
            }
        }
    }

} // namespace sgipc
