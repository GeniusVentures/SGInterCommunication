#include "sgipc/libp2p_ipc.hpp"
#include "ipc_messages.pb.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <cassert>
#include <chrono>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment( lib, "ws2_32.lib" )
#undef SendMessage  // Undefine Windows macro that conflicts with protobuf
#undef SendMessageA
#undef SendMessageW
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
        
        // TODO: Initialize libp2p node
        // For now, this is a stub implementation
        
        m_initialized.store( true );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::Shutdown()
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::SUCCESS;
        }

        m_shutdownRequested.store( true );
        m_listening.store( false );

        // Wait for threads to finish
        if ( m_heartbeatThread.joinable() )
        {
            m_heartbeatThread.join();
        }
        if ( m_cleanupThread.joinable() )
        {
            m_cleanupThread.join();
        }

        // TODO: Cleanup libp2p resources

        m_initialized.store( false );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::SendHeartbeat( uint16_t port )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        proto::IPCMessage message;
        message.set_type( proto::MessageType::HEARTBEAT );
        message.set_sender_id( m_config.instanceId );
        message.set_timestamp( std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch() )
                                   .count() );

        auto *heartbeat = message.mutable_heartbeat();
        heartbeat->set_instance_id( m_config.instanceId );
        heartbeat->set_port( port );
        heartbeat->set_timestamp( std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::system_clock::now().time_since_epoch() )
                                      .count() );

        // TODO: Send via libp2p
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::NegotiatePort( uint16_t preferredPort, uint16_t &assignedPort )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // TODO: Implement proper port negotiation
        // For now, just check if the preferred port is available
        if ( IsPortAvailable( preferredPort ) )
        {
            m_assignedPort.store( preferredPort );
            assignedPort = preferredPort;
        }
        else
        {
            // Find alternative port
            uint16_t altPort = FindAvailablePort( preferredPort );
            if ( altPort == 0 )
            {
                return IPCStatus::FAILED_TO_INITIALIZE;
            }
            m_assignedPort.store( altPort );
            assignedPort = altPort;
        }

        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::ListenForMessages( MessageCallback callback )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        if ( m_listening.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE; // Use a generic error for now
        }

        m_messageCallback = callback;
        m_listening.store( true );

        // Start background threads
        m_heartbeatThread = std::thread( &Libp2pIPC::HeartbeatSenderThread, this );
        m_cleanupThread   = std::thread( &Libp2pIPC::PeerCleanupThread, this );

        // TODO: Start libp2p listening

        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::StopListening()
    {
        m_listening.store( false );
        return IPCStatus::SUCCESS;
    }

    IPCStatus Libp2pIPC::SendMessage( const proto::IPCMessage &message, const std::string &recipientId )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // TODO: Serialize and send via libp2p
        std::string serialized;
        if ( !message.SerializeToString( &serialized ) )
        {
            return IPCStatus::FAILED_TO_SEND;
        }

        // TODO: Send serialized message via libp2p pubsub
        return IPCStatus::SUCCESS;
    }

    std::vector<proto::DiscoveryAnnouncement> Libp2pIPC::GetDiscoveredInstances() const
    {
        std::lock_guard<std::mutex> lock( m_peersMutex );
        std::vector<proto::DiscoveryAnnouncement> result;

        for ( const auto &[peerId, peer] : m_discoveredPeers )
        {
            result.push_back( peer.announcement );
        }

        return result;
    }

    bool Libp2pIPC::IsAvailable() const
    {
        // TODO: Check if libp2p is available
        // For now, assume it's available
        return true;
    }

    std::string Libp2pIPC::GetBackendName() const
    {
        return "libp2p";
    }

    const IPCConfig &Libp2pIPC::GetConfig() const
    {
        return m_config;
    }

    uint16_t Libp2pIPC::GetAssignedPort() const
    {
        return m_assignedPort.load();
    }

    // Private methods

    bool Libp2pIPC::InitializeLibp2pNode()
    {
        // TODO: Initialize libp2p node
        return true;
    }

    bool Libp2pIPC::SetupGossipSubTopic()
    {
        // TODO: Setup GossipSub topic
        return true;
    }

    bool Libp2pIPC::StartDiscoveryService()
    {
        // TODO: Start discovery service
        return true;
    }

    void Libp2pIPC::HandlePubSubMessage( const std::string          &topic,
                                         const std::vector<uint8_t> &data,
                                         const std::string          &sender )
    {
        if ( !m_listening.load() || !m_messageCallback )
        {
            return;
        }

        // TODO: Deserialize message from data
        proto::IPCMessage message;
        if ( !message.ParseFromArray( data.data(), static_cast<int>( data.size() ) ) )
        {
            return;
        }

        // TODO: Check if message is fresh
        
        // Handle heartbeat messages for peer discovery
        if ( message.type() == proto::MessageType::HEARTBEAT )
        {
            const auto &heartbeat = message.heartbeat();
            
            proto::DiscoveryAnnouncement announcement;
            announcement.set_instance_id( heartbeat.instance_id() );
            announcement.set_port( heartbeat.port() );
            announcement.set_timestamp( heartbeat.timestamp() );

            UpdatePeer( sender, announcement );
        }

        // Forward to callback
        m_messageCallback( message, sender );
    }

    void Libp2pIPC::HeartbeatSenderThread()
    {
        while ( !m_shutdownRequested.load() && m_listening.load() )
        {
            SendHeartbeat( m_assignedPort.load() );

            // Sleep for heartbeat interval
            std::this_thread::sleep_for( m_config.heartbeatInterval );
        }
    }

    void Libp2pIPC::PeerCleanupThread()
    {
        while ( !m_shutdownRequested.load() )
        {
            RemoveStalePeers();

            // Sleep for cleanup interval
            std::this_thread::sleep_for( CLEANUP_INTERVAL );
        }
    }

    uint16_t Libp2pIPC::FindAvailablePort( uint16_t preferredPort )
    {
        // Try preferred port first
        if ( IsPortAvailable( preferredPort ) )
        {
            return preferredPort;
        }

        // Try ports in a range around the preferred port
        const uint16_t range = 1000;
        const uint16_t start = (std::max)( 1024, static_cast<int>( preferredPort ) - range / 2 );
        const uint16_t end   = (std::min)( 65535, static_cast<int>( preferredPort ) + range / 2 );

        for ( uint16_t port = start; port <= end; ++port )
        {
            if ( IsPortAvailable( port ) )
            {
                return port;
            }
        }

        return 0; // No available port found
    }

    bool Libp2pIPC::IsPortAvailable( uint16_t port )
    {
        // TODO: Implement proper port availability check
        // This is a simplified stub implementation
#ifdef _WIN32
        WSADATA wsaData;
        if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
        {
            return false;
        }

        SOCKET sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock == INVALID_SOCKET )
        {
            WSACleanup();
            return false;
        }

        sockaddr_in addr = {};
        addr.sin_family  = AF_INET;
        addr.sin_port    = htons( port );
        addr.sin_addr.s_addr = INADDR_ANY;

        bool available = bind( sock, reinterpret_cast<sockaddr *>( &addr ), sizeof( addr ) ) == 0;

        closesocket( sock );
        WSACleanup();
        return available;
#else
        int sock = socket( AF_INET, SOCK_STREAM, 0 );
        if ( sock < 0 )
        {
            return false;
        }

        sockaddr_in addr = {};
        addr.sin_family  = AF_INET;
        addr.sin_port    = htons( port );
        addr.sin_addr.s_addr = INADDR_ANY;

        bool available = bind( sock, reinterpret_cast<sockaddr *>( &addr ), sizeof( addr ) ) == 0;

        close( sock );
        return available;
#endif
    }

    void Libp2pIPC::UpdatePeer( const std::string &peerId, const proto::DiscoveryAnnouncement &announcement )
    {
        std::lock_guard<std::mutex> lock( m_peersMutex );

        Peer &peer              = m_discoveredPeers[peerId];
        peer.peerId             = peerId;
        peer.port               = announcement.port();
        peer.last_seen          = std::chrono::steady_clock::now();
        peer.announcement       = announcement;
    }

    void Libp2pIPC::RemoveStalePeers()
    {
        std::lock_guard<std::mutex> lock( m_peersMutex );

        auto now = std::chrono::steady_clock::now();
        for ( auto it = m_discoveredPeers.begin(); it != m_discoveredPeers.end(); )
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
