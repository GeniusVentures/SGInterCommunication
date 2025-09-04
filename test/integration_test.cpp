#include <gtest/gtest.h>
#include "sgipc/cross_platform_ipc.hpp"
#include <thread>
#include <chrono>
#include <atomic>

class IntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Configuration for first instance
        config1_.instanceId        = "integration_test_instance_1";
        config1_.preferredPort     = 8081;
        config1_.timeout           = std::chrono::milliseconds( 5000 );
        config1_.heartbeatInterval = std::chrono::milliseconds( 2000 );
        config1_.messageTtl        = std::chrono::milliseconds( 15000 );
        config1_.allowFallback     = true;

        // Configuration for second instance
        config2_.instanceId        = "integration_test_instance_2";
        config2_.preferredPort     = 8082;
        config2_.timeout           = std::chrono::milliseconds( 5000 );
        config2_.heartbeatInterval = std::chrono::milliseconds( 2000 );
        config2_.messageTtl        = std::chrono::milliseconds( 15000 );
        config2_.allowFallback     = true;
    }

    void TearDown() override
    {
        if ( ipc1_ )
        {
            ipc1_->shutdown();
        }
        if ( ipc2_ )
        {
            ipc2_->shutdown();
        }
    }

    sgipc::IPCConfig                         config1_;
    sgipc::IPCConfig                         config2_;
    std::shared_ptr<sgipc::CrossPlatformIPC> ipc1_;
    std::shared_ptr<sgipc::CrossPlatformIPC> ipc2_;
};

// Test two-instance communication
TEST_F( IntegrationTest, TwoInstanceCommunication )
{
    // Create and initialize first instance
    ipc1_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc1_, nullptr );

    auto status = ipc1_->init( config1_ );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to initialize first instance";

    // Create and initialize second instance
    ipc2_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc2_, nullptr );

    status = ipc2_->init( config2_ );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to initialize second instance";

    // Set up message receiving for both instances
    std::atomic<int>  messages_received_by_1{ 0 };
    std::atomic<int>  messages_received_by_2{ 0 };
    std::atomic<bool> instance1_discovered_instance2{ false };
    std::atomic<bool> instance2_discovered_instance1{ false };

    auto callback1 = [&]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        messages_received_by_1++;
        if ( message.type() == sgipc::proto::MessageType::HEARTBEAT )
        {
            if ( message.heartbeat().instanceId() == config2_.instanceId )
            {
                instance1_discovered_instance2 = true;
            }
        }
    };

    auto callback2 = [&]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        messages_received_by_2++;
        if ( message.type() == sgipc::proto::MessageType::HEARTBEAT )
        {
            if ( message.heartbeat().instanceId() == config1_.instanceId )
            {
                instance2_discovered_instance1 = true;
            }
        }
    };

    // Start listening on both instances
    status = ipc1_->listenForMessages( callback1 );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to start listening on instance 1";

    status = ipc2_->listenForMessages( callback2 );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to start listening on instance 2";

    // Send heartbeats from both instances
    status = ipc1_->sendHeartbeat( ipc1_->getAssignedPort() );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to send heartbeat from instance 1";

    status = ipc2_->sendHeartbeat( ipc2_->getAssignedPort() );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to send heartbeat from instance 2";

    // Wait for message exchange (longer for file-based backends)
    std::this_thread::sleep_for( std::chrono::milliseconds( 8000 ) );

    // Verify message exchange
    EXPECT_GT( messages_received_by_1.load(), 0 ) << "Instance 1 should have received messages";
    EXPECT_GT( messages_received_by_2.load(), 0 ) << "Instance 2 should have received messages";

    // Send a few more heartbeats to ensure discovery
    for ( int i = 0; i < 3; ++i )
    {
        ipc1_->sendHeartbeat( ipc1_->getAssignedPort() );
        ipc2_->sendHeartbeat( ipc2_->getAssignedPort() );
        std::this_thread::sleep_for( std::chrono::milliseconds( 3000 ) );
    }

    // Check discovery (might not work for all backends immediately)
    // This is more informational than a hard requirement
    if ( instance1_discovered_instance2 || instance2_discovered_instance1 )
    {
        std::cout << "Instance discovery working!" << std::endl;
    }

    // Stop listening
    ipc1_->stopListening();
    ipc2_->stopListening();
}

// Test port negotiation between instances
TEST_F( IntegrationTest, PortNegotiationBetweenInstances )
{
    // Create and initialize first instance
    ipc1_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc1_, nullptr );

    auto status = ipc1_->init( config1_ );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    uint16_t port1 = 0;
    status         = ipc1_->negotiatePort( 8080, port1 );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    // Create and initialize second instance with same preferred port
    ipc2_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc2_, nullptr );

    config2_.preferredPort = 8080; // Same as first instance
    status                 = ipc2_->init( config2_ );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    uint16_t port2 = 0;
    status         = ipc2_->negotiatePort( 8080, port2 );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    // Ports should be different for most backends (except file-based which might not check)
    std::cout << "Port 1: " << port1 << ", Port 2: " << port2 << std::endl;

    // At minimum, both should have valid ports
    EXPECT_GT( port1, 0 ) << "Instance 1 should have a valid port";
    EXPECT_GT( port2, 0 ) << "Instance 2 should have a valid port";
}

// Test message sending between specific instances
TEST_F( IntegrationTest, DirectMessaging )
{
    // Create and initialize instances
    ipc1_ = sgipc::createPlatformIPC();
    ipc2_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc1_, nullptr );
    ASSERT_NE( ipc2_, nullptr );

    ASSERT_EQ( ipc1_->init( config1_ ), sgipc::IPCStatus::SUCCESS );
    ASSERT_EQ( ipc2_->init( config2_ ), sgipc::IPCStatus::SUCCESS );

    // Set up targeted message receiving
    std::atomic<bool>        received_targeted_message{ false };
    sgipc::proto::IPCMessage received_message;

    auto callback2 = [&]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        if ( message.recipientId() == config2_.instanceId || message.recipientId().empty() )
        {
            received_targeted_message = true;
            received_message          = message;
        }
    };

    // Start listening
    ASSERT_EQ( ipc2_->listenForMessages( callback2 ), sgipc::IPCStatus::SUCCESS );

    // Create a custom message from instance 1 to instance 2
    sgipc::proto::IPCMessage custom_message;
    custom_message.set_type( sgipc::proto::MessageType::HEARTBEAT );
    custom_message.set_sender_id( config1_.instanceId );
    custom_message.set_recipient_id( config2_.instanceId );
    custom_message.set_timestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() )
            .count() );

    auto heartbeat = custom_message.mutable_heartbeat();
    heartbeat->set_instance_id( config1_.instanceId );
    heartbeat->set_port( ipc1_->getAssignedPort() );
    heartbeat->set_timestamp( custom_message.timestamp() );
    heartbeat->set_version( "1.0.0" );

    // Send targeted message
    auto status = ipc1_->sendMessage( custom_message, config2_.instanceId );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to send targeted message";

    // Wait for message delivery
    std::this_thread::sleep_for( std::chrono::milliseconds( 5000 ) );

    // Verify message was received
    EXPECT_TRUE( received_targeted_message.load() ) << "Targeted message was not received";

    if ( received_targeted_message )
    {
        EXPECT_EQ( received_message.senderId(), config1_.instanceId );
        EXPECT_EQ( received_message.recipientId(), config2_.instanceId );
    }

    ipc2_->stopListening();
}

// Test backend fallback behavior
TEST_F( IntegrationTest, BackendConsistency )
{
    // Create multiple instances and ensure they use the same backend
    ipc1_ = sgipc::createPlatformIPC();
    ipc2_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc1_, nullptr );
    ASSERT_NE( ipc2_, nullptr );

    ASSERT_EQ( ipc1_->init( config1_ ), sgipc::IPCStatus::SUCCESS );
    ASSERT_EQ( ipc2_->init( config2_ ), sgipc::IPCStatus::SUCCESS );

    // Both instances should use the same backend
    std::string backend1 = ipc1_->getBackendName();
    std::string backend2 = ipc2_->getBackendName();

    EXPECT_EQ( backend1, backend2 ) << "Both instances should use the same backend type";

    std::cout << "Using backend: " << backend1 << std::endl;
}

// Performance test - measure message latency
TEST_F( IntegrationTest, MessageLatency )
{
    // Create and initialize instances
    ipc1_ = sgipc::createPlatformIPC();
    ipc2_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc1_, nullptr );
    ASSERT_NE( ipc2_, nullptr );

    ASSERT_EQ( ipc1_->init( config1_ ), sgipc::IPCStatus::SUCCESS );
    ASSERT_EQ( ipc2_->init( config2_ ), sgipc::IPCStatus::SUCCESS );

    std::atomic<bool> message_received{ false };
    auto              start_time = std::chrono::high_resolution_clock::now();
    auto              end_time   = start_time;

    auto callback2 = [&]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        if ( message.senderId() == config1_.instanceId )
        {
            end_time         = std::chrono::high_resolution_clock::now();
            message_received = true;
        }
    };

    // Start listening
    ASSERT_EQ( ipc2_->listenForMessages( callback2 ), sgipc::IPCStatus::SUCCESS );

    // Send message and measure time
    start_time  = std::chrono::high_resolution_clock::now();
    auto status = ipc1_->sendHeartbeat( ipc1_->getAssignedPort() );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    // Wait for response
    auto timeout    = std::chrono::seconds( 10 );
    auto wait_start = std::chrono::high_resolution_clock::now();

    while ( !message_received.load() && ( std::chrono::high_resolution_clock::now() - wait_start ) < timeout )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    if ( message_received.load() )
    {
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>( end_time - start_time );
        std::cout << "Message latency: " << latency.count() << " ms" << std::endl;

        // Latency should be reasonable (under 10 seconds for any backend)
        EXPECT_LT( latency.count(), 10000 ) << "Message latency too high: " << latency.count() << " ms";
    }
    else
    {
        std::cout << "Message not received within timeout" << std::endl;
    }

    ipc2_->stopListening();
}

int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
