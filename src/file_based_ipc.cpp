#include "sgipc/file_based_ipc.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#undef SendMessage  // Avoid Windows macro conflict
#else
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#endif

namespace sgipc
{

    FileBasedIPC::FileBasedIPC() = default;

    FileBasedIPC::~FileBasedIPC()
    {
        Shutdown();
    }

    IPCStatus FileBasedIPC::Init( const IPCConfig &config )
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

        // Initialize file system
        if ( !InitializeFileSystem() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Find and assign port (simplified for file-based IPC)
        m_assignedPort.store( m_config.preferredPort > 0 ? m_config.preferredPort : 8080 );

        m_initialized.store( true );
        m_shutdownRequested.store( false );

        return IPCStatus::SUCCESS;
    }

    IPCStatus FileBasedIPC::Shutdown()
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::SUCCESS;
        }

        m_shutdownRequested.store( true );
        m_listening.store( false );

        // Stop file watching thread
        if ( m_fileWatchingThread.joinable() )
        {
            m_fileWatchingThread.join();
        }

        // Cleanup old files
        CleanupOldFiles();

        m_initialized.store( false );
        return IPCStatus::SUCCESS;
    }

    IPCStatus FileBasedIPC::SendHeartbeat( uint16_t port )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

#ifdef SGIPC_MINIMAL_BUILD
        auto message = simple::createHeartbeat( m_config.instanceId );
        message.timestamp = GetCurrentTimestamp();
        message.payload = std::to_string( port ); // Store port in payload for simple messages
#else
        proto::IPCMessage message;
        message.set_type( proto::MessageType::HEARTBEAT );
        message.set_sender_id( m_config.instanceId );
        message.set_timestamp( GetCurrentTimestamp() );

        auto heartbeat = message.mutable_heartbeat();
        heartbeat->set_instance_id( m_config.instanceId );
        heartbeat->set_port( port );
        heartbeat->set_timestamp( GetCurrentTimestamp() );
        heartbeat->set_version( "1.0.0" );
#endif

        // Write heartbeat to file
        std::vector<uint8_t> serialized;
        if ( !SerializeMessage( message, serialized ) )
        {
            return IPCStatus::INVALID_MESSAGE;
        }

        std::string filename = m_heartbeatDirectory + "/" + GenerateMessageFilename( "heartbeat" );

        if ( !writeMessageToFile( filename, serialized ) )
        {
            return IPCStatus::FAILED_TO_SEND;
        }

        return IPCStatus::SUCCESS;
    }

    IPCStatus FileBasedIPC::NegotiatePort( uint16_t preferredPort, uint16_t &assignedPort )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // For file-based IPC, port negotiation is simplified
        // We just assign the preferred port or increment if needed
        assignedPort = preferredPort > 0 ? preferredPort : 8080;
        m_assignedPort.store( assignedPort );

        return IPCStatus::SUCCESS;
    }

    IPCStatus FileBasedIPC::ListenForMessages( MessageCallback callback )
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

        // Start file watching thread
        m_fileWatchingThread = std::thread( &FileBasedIPC::FileWatchingThread, this );

        return IPCStatus::SUCCESS;
    }

    IPCStatus FileBasedIPC::StopListening()
    {
        m_listening.store( false );
        return IPCStatus::SUCCESS;
    }

#ifdef SGIPC_MINIMAL_BUILD
    IPCStatus FileBasedIPC::SendMessage( const simple::SimpleMessage &message, const std::string &recipientId )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Serialize message
        std::vector<uint8_t> serialized;
        if ( !SerializeMessage( message, serialized ) )
        {
            return IPCStatus::INVALID_MESSAGE;
        }

        // Determine target directory based on message type
        std::string target_dir   = m_messagesDirectory;
        std::string message_type = "message";

        switch ( message.type )
        {
            case simple::MessageType::HEARTBEAT:
                target_dir   = m_heartbeatDirectory;
                message_type = "heartbeat";
                break;
            case simple::MessageType::DISCOVERY_ANNOUNCEMENT:
                target_dir   = m_discoveryDirectory;
                message_type = "discovery";
                break;
            default:
                break;
        }

        std::string filename = target_dir + "/" + GenerateMessageFilename( message_type );

        if ( !writeMessageToFile( filename, serialized ) )
        {
            return IPCStatus::FAILED_TO_SEND;
        }

        return IPCStatus::SUCCESS;
    }
#else
    IPCStatus FileBasedIPC::SendMessage( const proto::IPCMessage &message, const std::string &recipientId )
    {
        if ( !m_initialized.load() )
        {
            return IPCStatus::FAILED_TO_INITIALIZE;
        }

        // Serialize message
        std::vector<uint8_t> serialized;
        if ( !SerializeMessage( message, serialized ) )
        {
            return IPCStatus::INVALID_MESSAGE;
        }

        // Determine target directory based on message type
        std::string target_dir   = m_messagesDirectory;
        std::string message_type = "message";

        switch ( message.type() )
        {
            case proto::MessageType::HEARTBEAT:
                target_dir   = m_heartbeatDirectory;
                message_type = "heartbeat";
                break;
            case proto::MessageType::PORT_REQUEST:
            case proto::MessageType::PORT_RESPONSE:
                target_dir   = m_discoveryDirectory;
                message_type = "discovery";
                break;
            default:
                break;
        }

        std::string filename = target_dir + "/" + GenerateMessageFilename( message_type );

        if ( !writeMessageToFile( filename, serialized ) )
        {
            return IPCStatus::FAILED_TO_SEND;
        }

        return IPCStatus::SUCCESS;
    }
#endif

#ifdef SGIPC_MINIMAL_BUILD
    std::vector<simple::SimpleMessage> FileBasedIPC::GetDiscoveredInstances() const
    {
        std::lock_guard<std::mutex> lock( m_instancesMutex );
        return m_discoveredInstances;
    }
#else
    std::vector<proto::DiscoveryAnnouncement> FileBasedIPC::GetDiscoveredInstances() const
    {
        std::lock_guard<std::mutex> lock( m_instancesMutex );
        return m_discoveredInstances;
    }
#endif

    bool FileBasedIPC::IsAvailable() const
    {
        // File-based IPC is available on all platforms
        return true;
    }

    std::string FileBasedIPC::GetBackendName() const
    {
        return "file-based";
    }

    const IPCConfig &FileBasedIPC::GetConfig() const
    {
        return m_config;
    }

    uint16_t FileBasedIPC::GetAssignedPort() const
    {
        return m_assignedPort.load();
    }

    // Private method implementations

    bool FileBasedIPC::InitializeFileSystem()
    {
        std::string temp_dir = GetTempDirectoryPath();
        if ( temp_dir.empty() )
        {
            return false;
        }

        m_ipcDirectory = temp_dir + "/" + IPC_DIR_NAME;

        if ( !createIPCDirectories( m_ipcDirectory ) )
        {
            return false;
        }

        m_messagesDirectory  = m_ipcDirectory + "/" + MESSAGES_DIR_NAME;
        m_heartbeatDirectory = m_ipcDirectory + "/" + HEARTBEAT_DIR_NAME;
        m_discoveryDirectory = m_ipcDirectory + "/" + DISCOVERY_DIR_NAME;
        m_locksDirectory     = m_ipcDirectory + "/" + LOCKS_DIR_NAME;

        return true;
    }

    std::string FileBasedIPC::GetTempDirectoryPath() const
    {
#ifdef _WIN32
        wchar_t temp_path[MAX_PATH];
        if ( GetTempPathW( MAX_PATH, temp_path ) == 0 )
        {
            return "";
        }

        // Convert wide string to narrow string
        std::wstring wide_path( temp_path );
        std::string  narrow_path( wide_path.begin(), wide_path.end() );

        // Remove trailing backslash
        if ( !narrow_path.empty() && narrow_path.back() == '\\' )
        {
            narrow_path.pop_back();
        }

        return narrow_path;
#else
        // Try environment variables first
        const char *temp_dir = getenv( "TMPDIR" );
        if ( !temp_dir )
        {
            temp_dir = getenv( "TMP" );
        }
        if ( !temp_dir )
        {
            temp_dir = getenv( "TEMP" );
        }
        if ( !temp_dir )
        {
            temp_dir = "/tmp";
        }

        return std::string( temp_dir );
#endif
    }

    bool FileBasedIPC::createIPCDirectories( const std::string &basePath )
    {
        try
        {
            std::filesystem::create_directories( basePath );
            std::filesystem::create_directories( basePath + "/" + MESSAGES_DIR_NAME );
            std::filesystem::create_directories( basePath + "/" + HEARTBEAT_DIR_NAME );
            std::filesystem::create_directories( basePath + "/" + DISCOVERY_DIR_NAME );
            std::filesystem::create_directories( basePath + "/" + LOCKS_DIR_NAME );
            return true;
        }
        catch ( const std::exception &e )
        {
            std::cerr << "Failed to create IPC directories: " << e.what() << std::endl;
            return false;
        }
    }

    bool FileBasedIPC::writeMessageToFile( const std::string &filename, const std::vector<uint8_t> &message )
    {
        // Acquire file lock
        if ( !acquireFileLock( filename ) )
        {
            return false;
        }

        try
        {
            std::ofstream file( filename, std::ios::binary );
            if ( !file.is_open() )
            {
                releaseFileLock( filename );
                return false;
            }

            file.write( reinterpret_cast<const char *>( message.data() ), message.size() );
            file.close();

            releaseFileLock( filename );
            return true;
        }
        catch ( const std::exception & )
        {
            releaseFileLock( filename );
            return false;
        }
    }

    bool FileBasedIPC::ReadMessageFromFile( const std::string &filename, std::vector<uint8_t> &message )
    {
        try
        {
            std::ifstream file( filename, std::ios::binary | std::ios::ate );
            if ( !file.is_open() )
            {
                return false;
            }

            std::streamsize size = file.tellg();
            file.seekg( 0, std::ios::beg );

            message.resize( size );
            if ( !file.read( reinterpret_cast<char *>( message.data() ), size ) )
            {
                return false;
            }

            return true;
        }
        catch ( const std::exception & )
        {
            return false;
        }
    }

    void FileBasedIPC::FileWatchingThread()
    {
        while ( !m_shutdownRequested.load() && m_listening.load() )
        {
            std::vector<std::string> newFiles;

            // Scan all directories for new files
            scanForNewFiles( m_messagesDirectory, newFiles );
            scanForNewFiles( m_heartbeatDirectory, newFiles );
            scanForNewFiles( m_discoveryDirectory, newFiles );

            // Process new files
            for ( const auto &filepath : newFiles )
            {
                if ( !IsFileProcessed( filepath ) )
                {
                    ProcessMessageFile( filepath );
                    MarkFileAsProcessed( filepath );
                }
            }

            // Periodic cleanup
            auto now = std::chrono::steady_clock::now();
            if ( now - m_lastCleanup > FILE_CLEANUP_INTERVAL )
            {
                CleanupOldFiles();
                m_lastCleanup = now;
            }

            std::this_thread::sleep_for( FILE_SCAN_INTERVAL );
        }
    }

    void FileBasedIPC::scanForNewFiles( const std::string &directory, std::vector<std::string> &newFiles )
    {
        try
        {
            if ( !std::filesystem::exists( directory ) )
            {
                return;
            }

            for ( const auto &entry : std::filesystem::directory_iterator( directory ) )
            {
                if ( entry.is_regular_file() )
                {
                    newFiles.push_back( entry.path().string() );
                }
            }
        }
        catch ( const std::exception & )
        {
            // Ignore errors during directory scanning
        }
    }

    void FileBasedIPC::ProcessMessageFile( const std::string &filepath )
    {
        if ( !m_messageCallback )
        {
            return;
        }

        std::vector<uint8_t> message_data;
        if ( !ReadMessageFromFile( filepath, message_data ) )
        {
            return;
        }

#ifdef SGIPC_MINIMAL_BUILD
        simple::SimpleMessage message;
        if ( !DeserializeMessage( message_data, message ) )
        {
            return;
        }

        // Validate message freshness
        if ( !IsMessageFresh( message.timestamp, m_config.messageTtl ) )
        {
            return;
        }

        // Don't process our own messages
        if ( message.sender_id == m_config.instanceId )
        {
            return;
        }

        // Update discovered instances if it's a heartbeat
        if ( message.type == simple::MessageType::HEARTBEAT )
        {
            auto announcement = simple::createDiscoveryAnnouncement( message.sender_id, message.payload );
            announcement.timestamp = message.timestamp;

            std::lock_guard<std::mutex> lock( m_instancesMutex );

            // Update or add to discovered instances
            auto it = std::find_if( m_discoveredInstances.begin(),
                                    m_discoveredInstances.end(),
                                    [&]( const simple::SimpleMessage &existing )
                                    { return existing.sender_id == announcement.sender_id; } );

            if ( it != m_discoveredInstances.end() )
            {
                *it = announcement;
            }
            else
            {
                m_discoveredInstances.push_back( announcement );
            }
        }
#else
        proto::IPCMessage message;
        if ( !DeserializeMessage( message_data, message ) )
        {
            return;
        }

        // Validate message freshness
        if ( !IsMessageFresh( message.timestamp(), m_config.messageTtl ) )
        {
            return;
        }

        // Don't process our own messages
        if ( message.sender_id() == m_config.instanceId )
        {
            return;
        }

        // Update discovered instances if it's a heartbeat
        if ( message.type() == proto::MessageType::HEARTBEAT )
        {
            const auto &heartbeat = message.heartbeat();

            proto::DiscoveryAnnouncement announcement;
            announcement.set_instance_id( heartbeat.instance_id() );
            announcement.set_port( heartbeat.port() );
            announcement.set_timestamp( heartbeat.timestamp() );

            std::lock_guard<std::mutex> lock( m_instancesMutex );

            // Update or add to discovered instances
            auto it = std::find_if( m_discoveredInstances.begin(),
                                    m_discoveredInstances.end(),
                                    [&]( const proto::DiscoveryAnnouncement &existing )
                                    { return existing.instance_id() == announcement.instance_id(); } );

            if ( it != m_discoveredInstances.end() )
            {
                *it = announcement;
            }
            else
            {
                m_discoveredInstances.push_back( announcement );
            }
        }
#endif

        // Call user callback
        m_messageCallback( message, "file:" + filepath );
    }

    void FileBasedIPC::CleanupOldFiles()
    {
        std::vector<std::string> directories = { m_messagesDirectory, m_heartbeatDirectory, m_discoveryDirectory };

        auto cutoff_time = std::chrono::system_clock::now() - FILE_TTL;

        for ( const auto &dir : directories )
        {
            try
            {
                if ( !std::filesystem::exists( dir ) )
                {
                    continue;
                }

                for ( const auto &entry : std::filesystem::directory_iterator( dir ) )
                {
                    if ( entry.is_regular_file() )
                    {
                        auto file_time = std::filesystem::last_write_time( entry );
                        auto sctp      = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            file_time - std::filesystem::file_time_type::clock::now() +
                            std::chrono::system_clock::now() );

                        if ( sctp < cutoff_time )
                        {
                            std::filesystem::remove( entry.path() );
                        }
                    }
                }
            }
            catch ( const std::exception & )
            {
                // Ignore cleanup errors
            }
        }

        // Cleanup processed files list
        {
            std::lock_guard<std::mutex> lock( m_processedFilesMutex );
            // Keep only recent entries (simplified cleanup)
            if ( m_processedFiles.size() > 1000 )
            {
                m_processedFiles.clear();
            }
        }
    }

    std::string FileBasedIPC::GenerateMessageFilename( const std::string &message_type ) const
    {
        auto now       = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() ).count();

        return m_config.instanceId + "_" + message_type + "_" + std::to_string( timestamp ) + ".msg";
    }

    bool FileBasedIPC::IsFileProcessed( const std::string &filename )
    {
        std::lock_guard<std::mutex> lock( m_processedFilesMutex );
        return m_processedFiles.find( filename ) != m_processedFiles.end();
    }

    void FileBasedIPC::MarkFileAsProcessed( const std::string &filename )
    {
        std::lock_guard<std::mutex> lock( m_processedFilesMutex );
        m_processedFiles.insert( filename );
    }

    bool FileBasedIPC::acquireFileLock( const std::string &filepath )
    {
        std::string lock_file = m_locksDirectory + "/" + std::filesystem::path( filepath ).filename().string() +
                                ".lock";

        // Simple file-based locking
        std::ofstream lock( lock_file );
        return lock.is_open();
    }

    void FileBasedIPC::releaseFileLock( const std::string &filepath )
    {
        std::string lock_file = m_locksDirectory + "/" + std::filesystem::path( filepath ).filename().string() +
                                ".lock";

        try
        {
            std::filesystem::remove( lock_file );
        }
        catch ( const std::exception & )
        {
            // Ignore lock release errors
        }
    }

} // namespace sgipc
