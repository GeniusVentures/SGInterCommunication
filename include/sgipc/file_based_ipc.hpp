#pragma once

#include "cross_platform_ipc.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <unordered_set>
#include <unordered_map>

namespace sgipc
{

    /**
 * @brief File-based IPC implementation (fallback for desktop platforms)
 * 
 * This implementation uses temporary files and file system watching for
 * inter-process communication when libp2p is not available. It provides
 * basic messaging capabilities using shared directories and file polling.
 */
    class FileBasedIPC : public CrossPlatformIPC
    {
    public:
        FileBasedIPC();
        ~FileBasedIPC() override;

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
        struct FileMessage
        {
            std::string                           filename;
            std::vector<uint8_t>                  data;
            std::chrono::system_clock::time_point timestamp;
            std::string                           senderId;
        };

        /**
     * @brief Initialize file-based IPC directories
     * @return true on success, false on failure
     */
        bool InitializeFileSystem();

        /**
     * @brief Get platform-specific temporary directory path
     * @return Temporary directory path
     */
        std::string GetTempDirectoryPath() const;

        /**
     * @brief Create IPC directories if they don't exist
     * @param basePath Base directory path
     * @return true on success, false on failure
     */
        bool createIPCDirectories( const std::string &basePath );

        /**
     * @brief Write message to file
     * @param filename File path
     * @param message Serialized message data
     * @return true on success, false on failure
     */
        bool writeMessageToFile( const std::string &filename, const std::vector<uint8_t> &message );

        /**
     * @brief Read message from file
     * @param filename File path
     * @param message Output message data
     * @return true on success, false on failure
     */
        bool ReadMessageFromFile( const std::string &filename, std::vector<uint8_t> &message );

        /**
     * @brief File watching thread function
     */
        void FileWatchingThread();

        /**
     * @brief Scan directory for new message files
     * @param directory Directory to scan
     * @param newFiles Output vector of new file paths
     */
        void scanForNewFiles( const std::string &directory, std::vector<std::string> &newFiles );

        /**
     * @brief Process a message file
     * @param filepath Path to message file
     */
        void ProcessMessageFile( const std::string &filepath );

        /**
     * @brief Clean up old message files
     */
        void CleanupOldFiles();

        /**
     * @brief Generate unique filename for message
     * @param message_type Type of message
     * @return Unique filename
     */
        std::string GenerateMessageFilename( const std::string &message_type ) const;

        /**
     * @brief Check if file has been processed before
     * @param filename File name
     * @return true if processed, false if new
     */
        bool IsFileProcessed( const std::string &filename );

        /**
     * @brief Mark file as processed
     * @param filename File name
     */
        void MarkFileAsProcessed( const std::string &filename );

        /**
     * @brief Create lock file to prevent concurrent access
     * @param filepath File path to lock
     * @return true if lock acquired, false otherwise
     */
        bool acquireFileLock( const std::string &filepath );

        /**
     * @brief Release file lock
     * @param filepath File path to unlock
     */
        void releaseFileLock( const std::string &filepath );

        // Configuration and state
        IPCConfig             m_config;
        std::atomic<bool>     m_initialized{ false };
        std::atomic<bool>     m_listening{ false };
        std::atomic<bool>     m_shutdownRequested{ false };
        std::atomic<uint16_t> m_assignedPort{ 0 };

        // File system paths
        std::string m_ipcDirectory;
        std::string m_messagesDirectory;
        std::string m_heartbeatDirectory;
        std::string m_discoveryDirectory;
        std::string m_locksDirectory;

        // Threading
        std::thread     m_fileWatchingThread;
        MessageCallback m_messageCallback;

        // File tracking
        mutable std::mutex                    m_processedFilesMutex;
        std::unordered_set<std::string>       m_processedFiles;
        std::chrono::steady_clock::time_point m_lastCleanup;

        // Discovered instances
        mutable std::mutex                        m_instancesMutex;
        std::vector<proto::DiscoveryAnnouncement> m_discoveredInstances;

        // Configuration constants
        static constexpr auto FILE_SCAN_INTERVAL    = std::chrono::milliseconds( 2000 );
        static constexpr auto FILE_CLEANUP_INTERVAL = std::chrono::minutes( 5 );
        static constexpr auto FILE_TTL              = std::chrono::minutes( 10 );
        static constexpr auto LOCK_TIMEOUT          = std::chrono::seconds( 5 );

        // Directory names
        static constexpr const char *IPC_DIR_NAME       = "genius_ipc";
        static constexpr const char *MESSAGES_DIR_NAME  = "messages";
        static constexpr const char *HEARTBEAT_DIR_NAME = "heartbeat";
        static constexpr const char *DISCOVERY_DIR_NAME = "discovery";
        static constexpr const char *LOCKS_DIR_NAME     = "locks";
    };

} // namespace sgipc
