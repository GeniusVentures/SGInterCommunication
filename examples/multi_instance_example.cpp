#include "sgipc/cross_platform_ipc.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

class MultiInstanceDemo
{
public:
    MultiInstanceDemo( const std::string &instance_name, uint16_t preferredPort ) :
        instance_name_( instance_name ), preferred_port_( preferredPort ), running_( false )
    {
    }

    bool initialize()
    {
        // Create IPC instance
        ipc_ = sgipc::createPlatformIPC();
        if ( !ipc_ )
        {
            std::cerr << "[" << instance_name_ << "] Failed to create IPC instance" << std::endl;
            return false;
        }

        std::cout << "[" << instance_name_ << "] Using backend: " << ipc_->getBackendName() << std::endl;

        // Configure IPC
        sgipc::IPCConfig config;
        config.instanceId        = instance_name_;
        config.preferredPort     = preferred_port_;
        config.timeout           = std::chrono::milliseconds( 5000 );
        config.heartbeatInterval = std::chrono::milliseconds( 5000 );
        config.messageTtl        = std::chrono::milliseconds( 20000 );
        config.allowFallback     = true;

        // Initialize IPC
        auto status = ipc_->init( config );
        if ( status != sgipc::IPCStatus::SUCCESS )
        {
            std::cerr << "[" << instance_name_ << "] Failed to initialize IPC: " << sgipc::toString( status )
                      << std::endl;
            return false;
        }

        // Negotiate port
        uint16_t assignedPort = 0;
        status                = ipc_->negotiatePort( preferred_port_, assignedPort );
        if ( status != sgipc::IPCStatus::SUCCESS )
        {
            std::cerr << "[" << instance_name_ << "] Failed to negotiate port: " << sgipc::toString( status )
                      << std::endl;
            return false;
        }

        m_assignedPort = assignedPort;
        std::cout << "[" << instance_name_ << "] Assigned port: " << m_assignedPort << std::endl;

        return true;
    }

    void start()
    {
        if ( !ipc_ )
        {
            return;
        }

        running_ = true;

        // Set up message callback
        auto callback = [this]( const sgipc::proto::IPCMessage &message, const std::string &sender )
        { handleMessage( message, sender ); };

        // Start listening
        auto status = ipc_->listenForMessages( callback );
        if ( status != sgipc::IPCStatus::SUCCESS )
        {
            std::cerr << "[" << instance_name_ << "] Failed to start listening: " << sgipc::toString( status )
                      << std::endl;
            return;
        }

        std::cout << "[" << instance_name_ << "] Started listening for messages..." << std::endl;

        // Start heartbeat thread
        m_heartbeatThread = std::thread( &MultiInstanceDemo::heartbeatLoop, this );

        std::cout << "[" << instance_name_ << "] Instance started successfully!" << std::endl;
    }

    void stop()
    {
        running_ = false;

        if ( m_heartbeatThread.joinable() )
        {
            m_heartbeatThread.join();
        }

        if ( ipc_ )
        {
            ipc_->stopListening();
            ipc_->shutdown();
        }

        std::cout << "[" << instance_name_ << "] Instance stopped." << std::endl;
    }

    void sendCustomMessage( const std::string &target_instance, const std::string &custom_data )
    {
        if ( !ipc_ || !running_ )
        {
            return;
        }

        // Create custom heartbeat message with metadata
        sgipc::proto::IPCMessage message;
        message.set_type( sgipc::proto::MessageType::HEARTBEAT );
        message.set_sender_id( instance_name_ );
        message.set_recipient_id( target_instance );
        message.set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() )
                .count() );

        auto heartbeat = message.mutable_heartbeat();
        heartbeat->set_instance_id( instance_name_ );
        heartbeat->set_port( m_assignedPort );
        heartbeat->set_timestamp( message.timestamp() );
        heartbeat->set_version( "1.0.0" );

        // Add custom data as metadata
        ( *heartbeat->mutable_metadata() )["custom_data"]  = custom_data;
        ( *heartbeat->mutable_metadata() )["message_type"] = "custom_message";

        auto status = ipc_->sendMessage( message, target_instance );
        if ( status == sgipc::IPCStatus::SUCCESS )
        {
            std::cout << "[" << instance_name_ << "] Sent custom message to " << target_instance << ": " << custom_data
                      << std::endl;
        }
        else
        {
            std::cerr << "[" << instance_name_ << "] Failed to send message: " << sgipc::toString( status )
                      << std::endl;
        }
    }

    std::vector<std::string> getDiscoveredInstances() const
    {
        if ( !ipc_ )
        {
            return {};
        }

        auto                     announcements = ipc_->getDiscoveredInstances();
        std::vector<std::string> instances;

        for ( const auto &announcement : announcements )
        {
            if ( announcement.instanceId() != instance_name_ )
            {
                instances.push_back( announcement.instanceId() );
            }
        }

        return instances;
    }

    const std::string &getInstanceName() const
    {
        return instance_name_;
    }

    bool isRunning() const
    {
        return running_;
    }

private:
    void handleMessage( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        messages_received_++;

        // Don't process our own messages
        if ( message.senderId() == instance_name_ )
        {
            return;
        }

        std::cout << "[" << instance_name_ << "] Received message #" << messages_received_.load() << " from "
                  << message.senderId() << std::endl;

        if ( message.type() == sgipc::proto::MessageType::HEARTBEAT )
        {
            const auto &heartbeat = message.heartbeat();

            // Check if it's a custom message
            auto metadata    = heartbeat.metadata();
            auto msg_type_it = metadata.find( "message_type" );
            if ( msg_type_it != metadata.end() && msg_type_it->second == "custom_message" )
            {
                auto custom_data_it = metadata.find( "custom_data" );
                if ( custom_data_it != metadata.end() )
                {
                    std::cout << "[" << instance_name_ << "] Custom message data: " << custom_data_it->second
                              << std::endl;
                }
            }
            else
            {
                std::cout << "[" << instance_name_ << "] Regular heartbeat from " << heartbeat.instanceId()
                          << " on port " << heartbeat.port() << std::endl;
            }
        }
    }

    void heartbeatLoop()
    {
        while ( running_ )
        {
            if ( ipc_ )
            {
                auto status = ipc_->sendHeartbeat( m_assignedPort );
                if ( status != sgipc::IPCStatus::SUCCESS )
                {
                    std::cerr << "[" << instance_name_ << "] Failed to send heartbeat: " << sgipc::toString( status )
                              << std::endl;
                }
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );
        }
    }

    std::string       instance_name_;
    uint16_t          preferred_port_;
    uint16_t          m_assignedPort = 0;
    std::atomic<bool> running_;
    std::atomic<int>  messages_received_{ 0 };

    std::shared_ptr<sgipc::CrossPlatformIPC> ipc_;
    std::thread                              m_heartbeatThread;
};

void printHelp()
{
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  list       - List discovered instances" << std::endl;
    std::cout << "  send <instance> <message> - Send custom message to instance" << std::endl;
    std::cout << "  help       - Show this help" << std::endl;
    std::cout << "  quit       - Exit the application" << std::endl;
    std::cout << "\nExample: send instance_2 Hello from instance_1!" << std::endl;
}

int main( int argc, char *argv[] )
{
    std::string instance_name = "multi_instance_demo";
    uint16_t    preferredPort = 8080;

    // Parse command line arguments
    if ( argc >= 2 )
    {
        instance_name = argv[1];
    }
    if ( argc >= 3 )
    {
        preferredPort = static_cast<uint16_t>( std::stoi( argv[2] ) );
    }

    std::cout << "SGInterCommunication Multi-Instance Example" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "Instance: " << instance_name << std::endl;
    std::cout << "Preferred Port: " << preferredPort << std::endl;
    std::cout << std::endl;

    // Create and initialize instance
    MultiInstanceDemo demo( instance_name, preferredPort );

    if ( !demo.initialize() )
    {
        std::cerr << "Failed to initialize instance" << std::endl;
        return 1;
    }

    demo.start();

    std::cout << "\nInstance is running. Type 'help' for commands or 'quit' to exit." << std::endl;
    std::cout << "Tip: Start another instance with: " << argv[0] << " instance_2 8081" << std::endl;
    std::cout << std::endl;

    // Interactive command loop
    std::string input;
    while ( demo.isRunning() )
    {
        std::cout << "[" << instance_name << "] > ";
        std::getline( std::cin, input );

        if ( input.empty() )
        {
            continue;
        }

        if ( input == "quit" || input == "exit" )
        {
            break;
        }
        else if ( input == "help" )
        {
            printHelp();
        }
        else if ( input == "list" )
        {
            auto instances = demo.getDiscoveredInstances();
            if ( instances.empty() )
            {
                std::cout << "No other instances discovered yet." << std::endl;
            }
            else
            {
                std::cout << "Discovered instances:" << std::endl;
                for ( const auto &instance : instances )
                {
                    std::cout << "  - " << instance << std::endl;
                }
            }
        }
        else if ( input.substr( 0, 4 ) == "send" )
        {
            // Parse send command: send <instance> <message>
            size_t first_space = input.find( ' ', 5 );
            if ( first_space != std::string::npos )
            {
                std::string target  = input.substr( 5, first_space - 5 );
                std::string message = input.substr( first_space + 1 );
                demo.sendCustomMessage( target, message );
            }
            else
            {
                std::cout << "Usage: send <instance> <message>" << std::endl;
            }
        }
        else
        {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }

    demo.stop();

    std::cout << "Example completed!" << std::endl;
    return 0;
}
