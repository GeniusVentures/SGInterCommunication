#include "sgipc/cross_platform_ipc.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

int main()
{
    std::cout << "SGInterCommunication Simple Example" << std::endl;
    std::cout << "====================================" << std::endl;

    // Create IPC instance
    auto ipc = sgipc::CreatePlatformIPC();
    if ( !ipc )
    {
        std::cerr << "Failed to create IPC instance" << std::endl;
        return 1;
    }

    std::cout << "Using backend: " << ipc->GetBackendName() << std::endl;

    // Configure IPC
    sgipc::IPCConfig config;
    config.instanceId        = "simple_example_instance";
    config.preferredPort     = 8080;
    config.timeout           = std::chrono::milliseconds( 5000 );
    config.heartbeatInterval = std::chrono::milliseconds( 3000 );
    config.messageTtl        = std::chrono::milliseconds( 15000 );
    config.allowFallback     = true;

    // Initialize IPC
    auto status = ipc->Init( config );
    if ( status != sgipc::IPCStatus::SUCCESS )
    {
        std::cerr << "Failed to initialize IPC: " << sgipc::ToString( status ) << std::endl;
        return 1;
    }

    std::cout << "IPC initialized successfully" << std::endl;
    std::cout << "Instance ID: " << ipc->GetConfig().instanceId << std::endl;

    // Negotiate port
    uint16_t assignedPort = 0;
    status                = ipc->NegotiatePort( config.preferredPort, assignedPort );
    if ( status != sgipc::IPCStatus::SUCCESS )
    {
        std::cerr << "Failed to negotiate port: " << sgipc::ToString( status ) << std::endl;
        return 1;
    }

    std::cout << "Assigned port: " << assignedPort << std::endl;

    // Set up message callback
    std::atomic<int> message_count{ 0 };
    std::atomic<int> heartbeat_count{ 0 };

#ifdef SGIPC_MINIMAL_BUILD
    auto message_callback = [&]( const sgipc::simple::SimpleMessage &message, const std::string &sender )
    {
        message_count++;

        std::cout << "Received message #" << message_count.load() << std::endl;
        std::cout << "  Type: " << static_cast<int>(message.type) << std::endl;
        std::cout << "  Sender: " << message.sender_id << std::endl;
        std::cout << "  Timestamp: " << message.timestamp << std::endl;
        std::cout << "  From: " << sender << std::endl;

        if ( message.type == sgipc::simple::MessageType::HEARTBEAT )
        {
            heartbeat_count++;
            std::cout << "  Heartbeat payload: " << message.payload << std::endl;
        }

        std::cout << std::endl;
    };
#else
    auto message_callback = [&]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        message_count++;

        std::cout << "Received message #" << message_count.load() << std::endl;
        std::cout << "  Type: " << static_cast<int>( message.type() ) << std::endl;
        std::cout << "  Sender: " << message.sender_id() << std::endl;
        std::cout << "  Timestamp: " << message.timestamp() << std::endl;
        std::cout << "  From: " << sender << std::endl;

        if ( message.type() == sgipc::proto::MessageType::HEARTBEAT )
        {
            heartbeat_count++;
            const auto &heartbeat = message.heartbeat();
            std::cout << "  Heartbeat from instance: " << heartbeat.instance_id() << std::endl;
            std::cout << "  Port: " << heartbeat.port() << std::endl;
            std::cout << "  Version: " << heartbeat.version() << std::endl;
        }

        std::cout << std::endl;
    };
#endif

    // Start listening for messages
    status = ipc->ListenForMessages( message_callback );
    if ( status != sgipc::IPCStatus::SUCCESS )
    {
        std::cerr << "Failed to start listening: " << sgipc::ToString( status ) << std::endl;
        return 1;
    }

    std::cout << "Started listening for messages..." << std::endl;

    // Send periodic heartbeats and run for a while
    std::cout << "Running for 30 seconds..." << std::endl;
    std::cout << "Press Ctrl+C to exit early" << std::endl;

    for ( int i = 0; i < 10; ++i )
    {
        // Send heartbeat
        status = ipc->SendHeartbeat( assignedPort );
        if ( status != sgipc::IPCStatus::SUCCESS )
        {
            std::cerr << "Failed to send heartbeat: " << sgipc::ToString( status ) << std::endl;
        }
        else
        {
            std::cout << "Sent heartbeat #" << ( i + 1 ) << std::endl;
        }

        // Wait 3 seconds
        std::this_thread::sleep_for( std::chrono::milliseconds( 3000 ) );

        // Show statistics
        std::cout << "Messages received: " << message_count.load() << " (Heartbeats: " << heartbeat_count.load() << ")"
                  << std::endl;

        // Show discovered instances
        auto instances = ipc->GetDiscoveredInstances();
        if ( !instances.empty() )
        {
            std::cout << "Discovered " << instances.size() << " instance(s):" << std::endl;
#ifdef SGIPC_MINIMAL_BUILD
            for ( const auto &instance : instances )
            {
                std::cout << "  - " << instance.sender_id << " payload: " << instance.payload << std::endl;
            }
#else
            for ( const auto &instance : instances )
            {
                std::cout << "  - " << instance.instance_id() << " on port " << instance.port() << std::endl;
            }
#endif
        }

        std::cout << "---" << std::endl;
    }

    // Stop listening
    status = ipc->StopListening();
    if ( status != sgipc::IPCStatus::SUCCESS )
    {
        std::cerr << "Failed to stop listening: " << sgipc::ToString( status ) << std::endl;
    }

    // Shutdown
    status = ipc->Shutdown();
    if ( status != sgipc::IPCStatus::SUCCESS )
    {
        std::cerr << "Failed to shutdown IPC: " << sgipc::ToString( status ) << std::endl;
    }

    std::cout << "Example completed successfully!" << std::endl;
    std::cout << "Final statistics:" << std::endl;
    std::cout << "  Total messages received: " << message_count.load() << std::endl;
    std::cout << "  Total heartbeats received: " << heartbeat_count.load() << std::endl;

    return 0;
}
