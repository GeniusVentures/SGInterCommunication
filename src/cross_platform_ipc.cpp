#include "sgipc/cross_platform_ipc.hpp"
#include "sgipc/libp2p_ipc.hpp"
#include "sgipc/file_based_ipc.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#include "sgipc/ios_pasteboard_ipc.hpp"
#endif
#endif

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace sgipc
{

    // CrossPlatformIPC protected method implementations

    bool CrossPlatformIPC::IsMessageFresh( uint64_t timestamp, std::chrono::milliseconds ttl ) const
    {
        auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch() )
                                .count();

        return ( current_time - timestamp ) <= ttl.count();
    }

    uint64_t CrossPlatformIPC::GetCurrentTimestamp() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch() )
            .count();
    }

    std::string CrossPlatformIPC::GenerateInstanceId() const
    {
        std::random_device              rd;
        std::mt19937                    gen( rd() );
        std::uniform_int_distribution<> dis( 0, 15 );

        std::stringstream ss;
        ss << "sgipc-";

        // Generate UUID-like string
        for ( int i = 0; i < 8; ++i )
        {
            ss << std::hex << dis( gen );
        }
        ss << "-";
        for ( int i = 0; i < 4; ++i )
        {
            ss << std::hex << dis( gen );
        }
        ss << "-";
        for ( int i = 0; i < 4; ++i )
        {
            ss << std::hex << dis( gen );
        }
        ss << "-";
        for ( int i = 0; i < 4; ++i )
        {
            ss << std::hex << dis( gen );
        }
        ss << "-";
        for ( int i = 0; i < 12; ++i )
        {
            ss << std::hex << dis( gen );
        }

        return ss.str();
    }

    bool CrossPlatformIPC::SerializeMessage( const proto::IPCMessage &message, std::vector<uint8_t> &output ) const
    {
        try
        {
            std::string serialized;
            if ( !message.SerializeToString( &serialized ) )
            {
                return false;
            }

            output.assign( serialized.begin(), serialized.end() );
            return true;
        }
        catch ( const std::exception & )
        {
            return false;
        }
    }

    bool CrossPlatformIPC::DeserializeMessage( const std::vector<uint8_t> &data, proto::IPCMessage &message ) const
    {
        try
        {
            std::string serialized( data.begin(), data.end() );
            return message.ParseFromString( serialized );
        }
        catch ( const std::exception & )
        {
            return false;
        }
    }

    // Factory function implementation
    std::shared_ptr<CrossPlatformIPC> CreatePlatformIPC()
    {
        // Try libp2p IPC first as the primary backend
        auto libp2p_ipc = std::make_shared<Libp2pIPC>();
        if ( libp2p_ipc->IsAvailable() )
        {
            return libp2p_ipc;
        }

        // Fallback to file-based IPC
        auto file_ipc = std::make_shared<FileBasedIPC>();
        if ( file_ipc->IsAvailable() )
        {
            return file_ipc;
        }

        // No backends available
        return nullptr;
    }

    // Status to string conversion
    std::string ToString( IPCStatus status )
    {
        switch ( status )
        {
            case IPCStatus::SUCCESS:
                return "Success";
            case IPCStatus::FAILED_TO_INITIALIZE:
                return "Failed to initialize";
            case IPCStatus::FAILED_TO_SEND:
                return "Failed to send message";
            case IPCStatus::FAILED_TO_RECEIVE:
                return "Failed to receive message";
            case IPCStatus::TIMEOUT:
                return "Operation timeout";
            case IPCStatus::INVALID_MESSAGE:
                return "Invalid message format";
            case IPCStatus::NETWORK_ERROR:
                return "Network error";
            case IPCStatus::PERMISSION_DENIED:
                return "Permission denied";
            case IPCStatus::BACKEND_NOT_AVAILABLE:
                return "Backend not available";
            default:
                return "Unknown error";
        }
    }

} // namespace sgipc
