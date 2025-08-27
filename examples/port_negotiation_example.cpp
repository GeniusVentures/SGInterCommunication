#include "sgipc/cross_platform_ipc.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

struct InstanceInfo
{
    std::string                              name;
    uint16_t                                 preferredPort;
    uint16_t                                 assignedPort = 0;
    std::shared_ptr<sgipc::CrossPlatformIPC> ipc;
    bool                                     initialized = false;
};

class PortNegotiationDemo
{
public:
    PortNegotiationDemo() = default;

    void addInstance( const std::string &name, uint16_t preferredPort )
    {
        InstanceInfo info;
        info.name          = name;
        info.preferredPort = preferredPort;
        instances_.push_back( std::move( info ) );
    }

    bool initializeAllInstances()
    {
        std::cout << "Initializing " << instances_.size() << " instances..." << std::endl;
        std::cout << "=============================================" << std::endl;

        for ( auto &instance : instances_ )
        {
            std::cout << "\nInitializing " << instance.name << " (preferred port: " << instance.preferredPort << ")"
                      << std::endl;

            // Create IPC instance
            instance.ipc = sgipc::createPlatformIPC();
            if ( !instance.ipc )
            {
                std::cerr << "Failed to create IPC for " << instance.name << std::endl;
                continue;
            }

            // Configure IPC
            sgipc::IPCConfig config;
            config.instanceId        = instance.name;
            config.preferredPort     = instance.preferredPort;
            config.timeout           = std::chrono::milliseconds( 3000 );
            config.heartbeatInterval = std::chrono::milliseconds( 10000 );
            config.messageTtl        = std::chrono::milliseconds( 15000 );
            config.allowFallback     = true;

            // Initialize IPC
            auto status = instance.ipc->init( config );
            if ( status != sgipc::IPCStatus::SUCCESS )
            {
                std::cerr << "Failed to initialize IPC for " << instance.name << ": " << sgipc::toString( status )
                          << std::endl;
                continue;
            }

            // Negotiate port
            status = instance.ipc->negotiatePort( instance.preferredPort, instance.assignedPort );
            if ( status != sgipc::IPCStatus::SUCCESS )
            {
                std::cerr << "Failed to negotiate port for " << instance.name << ": " << sgipc::toString( status )
                          << std::endl;
                continue;
            }

            instance.initialized = true;

            std::cout << "✓ " << instance.name << " initialized successfully" << std::endl;
            std::cout << "  Backend: " << instance.ipc->getBackendName() << std::endl;
            std::cout << "  Preferred port: " << instance.preferredPort << std::endl;
            std::cout << "  Assigned port: " << instance.assignedPort << std::endl;

            // Small delay between initializations
            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        }

        return true;
    }

    void showPortAllocation()
    {
        std::cout << "\n\nPort Allocation Summary" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "Instance Name      | Preferred | Assigned | Status" << std::endl;
        std::cout << "------------------ | --------- | -------- | --------" << std::endl;

        for ( const auto &instance : instances_ )
        {
            std::string status = instance.initialized ? "Success" : "Failed";

            printf( "%-18s | %9d | %8d | %s\n",
                    instance.name.c_str(),
                    instance.preferredPort,
                    instance.assignedPort,
                    status.c_str() );
        }

        // Analyze port conflicts
        analyzePortConflicts();
    }

    void testCommunication()
    {
        std::cout << "\n\nTesting Inter-Instance Communication" << std::endl;
        std::cout << "====================================" << std::endl;

        // Set up message callbacks for all instances
        for ( auto &instance : instances_ )
        {
            if ( !instance.initialized )
            {
                continue;
            }

            auto callback = [&instance]( const sgipc::proto::IPCMessage &message, const std::string &sender )
            {
                if ( message.senderId() != instance.name )
                {
                    std::cout << "[" << instance.name << "] Received message from " << message.senderId() << " (port "
                              << message.heartbeat().port() << ")" << std::endl;
                }
            };

            auto status = instance.ipc->listenForMessages( callback );
            if ( status != sgipc::IPCStatus::SUCCESS )
            {
                std::cerr << "Failed to start listening for " << instance.name << std::endl;
            }
        }

        // Send heartbeats from all instances
        std::cout << "\nSending heartbeats from all instances..." << std::endl;
        for ( auto &instance : instances_ )
        {
            if ( !instance.initialized )
            {
                continue;
            }

            auto status = instance.ipc->sendHeartbeat( instance.assignedPort );
            if ( status == sgipc::IPCStatus::SUCCESS )
            {
                std::cout << "✓ Heartbeat sent from " << instance.name << std::endl;
            }
            else
            {
                std::cerr << "✗ Failed to send heartbeat from " << instance.name << ": " << sgipc::toString( status )
                          << std::endl;
            }
        }

        // Wait for message exchange
        std::cout << "\nWaiting for message exchange (10 seconds)..." << std::endl;
        std::this_thread::sleep_for( std::chrono::milliseconds( 10000 ) );

        // Show discovery results
        showDiscoveryResults();

        // Stop listening
        for ( auto &instance : instances_ )
        {
            if ( instance.initialized )
            {
                instance.ipc->stopListening();
            }
        }
    }

    void shutdownAllInstances()
    {
        std::cout << "\n\nShutting down all instances..." << std::endl;
        for ( auto &instance : instances_ )
        {
            if ( instance.initialized )
            {
                instance.ipc->shutdown();
                std::cout << "✓ " << instance.name << " shutdown complete" << std::endl;
            }
        }
    }

private:
    void analyzePortConflicts()
    {
        std::cout << "\nPort Conflict Analysis:" << std::endl;

        // Check for identical preferred ports with different assigned ports
        std::map<uint16_t, std::vector<std::string>> preferred_port_map;
        std::map<uint16_t, std::vector<std::string>> m_assignedPortmap;

        for ( const auto &instance : instances_ )
        {
            if ( !instance.initialized )
            {
                continue;
            }

            preferred_port_map[instance.preferredPort].push_back( instance.name );
            m_assignedPortmap[instance.assignedPort].push_back( instance.name );
        }

        // Report preferred port conflicts
        for ( const auto &[port, instances] : preferred_port_map )
        {
            if ( instances.size() > 1 )
            {
                std::cout << "• Multiple instances wanted port " << port << ": ";
                for ( size_t i = 0; i < instances.size(); ++i )
                {
                    std::cout << instances[i];
                    if ( i < instances.size() - 1 )
                    {
                        std::cout << ", ";
                    }
                }
                std::cout << std::endl;
            }
        }

        // Report assigned port conflicts (should not happen)
        for ( const auto &[port, instances] : m_assignedPortmap )
        {
            if ( instances.size() > 1 )
            {
                std::cout << "⚠ WARNING: Multiple instances assigned same port " << port << ": ";
                for ( size_t i = 0; i < instances.size(); ++i )
                {
                    std::cout << instances[i];
                    if ( i < instances.size() - 1 )
                    {
                        std::cout << ", ";
                    }
                }
                std::cout << std::endl;
            }
        }

        // Check how many got their preferred port
        int got_preferred = 0;
        for ( const auto &instance : instances_ )
        {
            if ( instance.initialized && instance.preferredPort == instance.assignedPort )
            {
                got_preferred++;
            }
        }

        std::cout << "• " << got_preferred << " out of " << instances_.size() << " instances got their preferred port"
                  << std::endl;
    }

    void showDiscoveryResults()
    {
        std::cout << "\nDiscovery Results:" << std::endl;

        for ( const auto &instance : instances_ )
        {
            if ( !instance.initialized )
            {
                continue;
            }

            auto discovered = instance.ipc->getDiscoveredInstances();
            std::cout << "[" << instance.name << "] Discovered " << discovered.size()
                      << " other instance(s):" << std::endl;

            for ( const auto &announcement : discovered )
            {
                if ( announcement.instanceId() != instance.name )
                {
                    std::cout << "  - " << announcement.instanceId() << " on port " << announcement.port() << std::endl;
                }
            }
        }
    }

    std::vector<InstanceInfo> instances_;
};

int main()
{
    std::cout << "SGInterCommunication Port Negotiation Example" << std::endl;
    std::cout << "=============================================" << std::endl;

    PortNegotiationDemo demo;

    // Add multiple instances with overlapping preferred ports
    demo.addInstance( "WebServer", 8080 );
    demo.addInstance( "APIServer", 8080 ); // Same preferred port
    demo.addInstance( "AdminPanel", 8081 );
    demo.addInstance( "MetricsService", 8080 ); // Same preferred port again
    demo.addInstance( "BackupService", 8082 );
    demo.addInstance( "MonitoringAgent", 8081 ); // Same as AdminPanel

    // Initialize all instances
    if ( !demo.initializeAllInstances() )
    {
        std::cerr << "Failed to initialize some instances" << std::endl;
        return 1;
    }

    // Show port allocation results
    demo.showPortAllocation();

    // Test communication between instances
    demo.testCommunication();

    // Shutdown
    demo.shutdownAllInstances();

    std::cout << "\nPort negotiation example completed!" << std::endl;
    std::cout << "\nKey Observations:" << std::endl;
    std::cout << "• Port conflicts are automatically resolved" << std::endl;
    std::cout << "• Each instance gets a unique port" << std::endl;
    std::cout << "• Instances can discover and communicate with each other" << std::endl;
    std::cout << "• The backend handles the complexity of port management" << std::endl;

    return 0;
}
