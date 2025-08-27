#include <gtest/gtest.h>
#include "sgipc/cross_platform_ipc.hpp"
#include <thread>
#include <chrono>

class SGIPCTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_config.instanceId = "test_instance_" +
                              std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() );
        m_config.preferredPort     = 8080;
        m_config.timeout           = std::chrono::milliseconds( 5000 );
        m_config.heartbeatInterval = std::chrono::milliseconds( 2000 );
        m_config.messageTtl        = std::chrono::milliseconds( 10000 );
        m_config.allowFallback     = true;
    }

    void TearDown() override
    {
        if ( ipc_ )
        {
            ipc_->shutdown();
        }
    }

    sgipc::IPCConfig                         m_config;
    std::shared_ptr<sgipc::CrossPlatformIPC> ipc_;
};

// Test IPC factory creation
TEST_F( SGIPCTest, CreatePlatformIPC )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr ) << "Failed to create platform IPC instance";
    EXPECT_FALSE( ipc_->getBackendName().empty() ) << "Backend name should not be empty";
}

// Test IPC initialization
TEST_F( SGIPCTest, InitializeIPC )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    auto status = ipc_->init( m_config );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "IPC initialization failed: " << sgipc::toString( status );

    // Check that configuration was set
    const auto &retrieved_config = ipc_->getConfig();
    EXPECT_EQ( retrieved_config.instanceId, m_config.instanceId );
    EXPECT_EQ( retrieved_config.preferredPort, m_config.preferredPort );
}

// Test port negotiation
TEST_F( SGIPCTest, PortNegotiation )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    auto status = ipc_->init( m_config );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    uint16_t assignedPort = 0;
    status                = ipc_->negotiatePort( m_config.preferredPort, assignedPort );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Port negotiation failed: " << sgipc::toString( status );
    EXPECT_GT( assignedPort, 0 ) << "Assigned port should be greater than 0";
    EXPECT_EQ( assignedPort, ipc_->getAssignedPort() ) << "Assigned port should match getter result";
}

// Test heartbeat sending
TEST_F( SGIPCTest, SendHeartbeat )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    auto status = ipc_->init( m_config );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    uint16_t port = 8080;
    status        = ipc_->sendHeartbeat( port );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Heartbeat sending failed: " << sgipc::toString( status );
}

// Test message listening
TEST_F( SGIPCTest, MessageListening )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    auto status = ipc_->init( m_config );
    ASSERT_EQ( status, sgipc::IPCStatus::SUCCESS );

    bool message_received = false;
    auto callback         = [&message_received]( const sgipc::proto::IPCMessage &message, const std::string &sender )
    {
        message_received = true;
        EXPECT_FALSE( message.senderId().empty() ) << "Sender ID should not be empty";
        EXPECT_GT( message.timestamp(), 0 ) << "Timestamp should be greater than 0";
    };

    status = ipc_->listenForMessages( callback );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to start listening: " << sgipc::toString( status );

    // Send a test message to ourselves
    sgipc::proto::IPCMessage test_message;
    test_message.set_type( sgipc::proto::MessageType::HEARTBEAT );
    test_message.set_sender_id( m_config.instanceId );
    test_message.set_timestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() )
            .count() );

    auto heartbeat = test_message.mutable_heartbeat();
    heartbeat->set_instance_id( m_config.instanceId );
    heartbeat->set_port( 8080 );
    heartbeat->set_timestamp( test_message.timestamp() );
    heartbeat->set_version( "1.0.0" );

    status = ipc_->sendMessage( test_message );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to send test message: " << sgipc::toString( status );

    // Give some time for message processing (especially for file-based backend)
    std::this_thread::sleep_for( std::chrono::milliseconds( 3000 ) );

    status = ipc_->stopListening();
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Failed to stop listening: " << sgipc::toString( status );
}

// Test status string conversion
TEST_F( SGIPCTest, StatusStringConversion )
{
    EXPECT_EQ( sgipc::toString( sgipc::IPCStatus::SUCCESS ), "Success" );
    EXPECT_EQ( sgipc::toString( sgipc::IPCStatus::FAILED_TO_INITIALIZE ), "Failed to initialize" );
    EXPECT_EQ( sgipc::toString( sgipc::IPCStatus::TIMEOUT ), "Operation timeout" );
    EXPECT_EQ( sgipc::toString( sgipc::IPCStatus::INVALID_MESSAGE ), "Invalid message format" );
}

// Test backend availability
TEST_F( SGIPCTest, BackendAvailability )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    EXPECT_TRUE( ipc_->isAvailable() ) << "Selected backend should be available";

    std::string backend_name = ipc_->getBackendName();
    EXPECT_FALSE( backend_name.empty() ) << "Backend name should not be empty";

    // Backend should be one of the known types
    EXPECT_TRUE( backend_name == "libp2p" || backend_name == "file-based" || backend_name == "ios-pasteboard" )
        << "Unknown backend: " << backend_name;
}

// Test message serialization/deserialization
TEST_F( SGIPCTest, MessageSerialization )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    // Create a test message
    sgipc::proto::IPCMessage original_message;
    original_message.set_type( sgipc::proto::MessageType::HEARTBEAT );
    original_message.set_sender_id( "test_sender" );
    original_message.set_timestamp( 1234567890 );

    auto heartbeat = original_message.mutable_heartbeat();
    heartbeat->set_instance_id( "test_instance" );
    heartbeat->set_port( 8080 );
    heartbeat->set_timestamp( 1234567890 );
    heartbeat->set_version( "1.0.0" );

    // Test serialization
    std::vector<uint8_t> serialized;
    bool                 result = ipc_->serializeMessage( original_message, serialized );
    EXPECT_TRUE( result ) << "Serialization should succeed";
    EXPECT_FALSE( serialized.empty() ) << "Serialized data should not be empty";

    // Test deserialization
    sgipc::proto::IPCMessage deserialized_message;
    result = ipc_->deserializeMessage( serialized, deserialized_message );
    EXPECT_TRUE( result ) << "Deserialization should succeed";

    // Verify message content
    EXPECT_EQ( deserialized_message.type(), original_message.type() );
    EXPECT_EQ( deserialized_message.senderId(), original_message.senderId() );
    EXPECT_EQ( deserialized_message.timestamp(), original_message.timestamp() );
    EXPECT_EQ( deserialized_message.heartbeat().instanceId(), original_message.heartbeat().instanceId() );
    EXPECT_EQ( deserialized_message.heartbeat().port(), original_message.heartbeat().port() );
}

// Test configuration validation
TEST_F( SGIPCTest, ConfigurationValidation )
{
    ipc_ = sgipc::createPlatformIPC();
    ASSERT_NE( ipc_, nullptr );

    // Test with empty instance ID (should be auto-generated)
    sgipc::IPCConfig empty_config;
    empty_config.preferredPort = 9090;

    auto status = ipc_->init( empty_config );
    EXPECT_EQ( status, sgipc::IPCStatus::SUCCESS ) << "Initialization with empty instance ID should succeed";

    const auto &retrieved_config = ipc_->getConfig();
    EXPECT_FALSE( retrieved_config.instanceId.empty() ) << "Instance ID should be auto-generated";
    EXPECT_EQ( retrieved_config.preferredPort, 9090 );
}

int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
