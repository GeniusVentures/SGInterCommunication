# SGIPC CMake Configuration Parameters
# Version configuration
set(CPACK_PACKAGE_VERSION_MAJOR "1" CACHE STRING "Package config major")
set(CPACK_PACKAGE_VERSION_MINOR "0" CACHE STRING "Package config minor") 
set(CPACK_PACKAGE_VERSION_PATCH "0" CACHE STRING "Package config patch")
set(CPACK_PACKAGE_VENDOR "Genius Ventures" CACHE STRING "The Package Vendor")

# BOOST VERSION TO USE
set(BOOST_MAJOR_VERSION "1" CACHE STRING "Boost Major Version")
set(BOOST_MINOR_VERSION "85" CACHE STRING "Boost Minor Version")
set(BOOST_PATCH_VERSION "0" CACHE STRING "Boost Patch Version")

# convenience settings
set(BOOST_VERSION "${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_PATCH_VERSION}")
set(BOOST_VERSION_2U "${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEFINED USE_BOOST_INCLUDE_POSTFIX)
        set(BOOST_INCLUDE_POSTFIX "/boost-${BOOST_VERSION_2U}" CACHE STRING "Boost include postfix")
endif()

# --------------------------------------------------------
# Set config of GTest (Optional)
set(GTest_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/lib/cmake/GTest")
set(GTest_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/include")
find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
    include_directories(${GTest_INCLUDE_DIR})
endif()

# --------------------------------------------------------
# Set config of Protobuf (Optional) - completely optional, ignore all errors
set(Protobuf_FOUND FALSE)
set(Protobuf_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/cmake")
set(Protobuf_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/include")
set(Protobuf_LIBRARY_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/lib")
find_program(PROTOC_EXECUTABLE NAMES protoc PATHS "${_THIRDPARTY_BUILD_DIR}/grpc/bin" QUIET)
if(PROTOC_EXECUTABLE)
    find_package(Protobuf QUIET)
    if(Protobuf_FOUND)
        message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
        set(Protobuf_PROTOC_EXECUTABLE ${PROTOC_EXECUTABLE})
        include_directories(${Protobuf_INCLUDE_DIR})
    endif()
endif()
if(NOT Protobuf_FOUND)
    message(STATUS "Protobuf not available - using stub headers")
endif()

# Boost should be loaded before libp2p v0.1.2
# --------------------------------------------------------
# Set config of Boost project
set(_BOOST_ROOT "${_THIRDPARTY_BUILD_DIR}/boost/build")
set(Boost_LIB_DIR "${_BOOST_ROOT}/lib")
set(Boost_INCLUDE_DIR "${_BOOST_ROOT}/include/boost-${BOOST_VERSION_2U}")
set(Boost_DIR "${Boost_LIB_DIR}/cmake/Boost-${BOOST_VERSION}")
set(boost_atomic_DIR "${Boost_LIB_DIR}/cmake/boost_atomic-${BOOST_VERSION}")
set(boost_chrono_DIR "${Boost_LIB_DIR}/cmake/boost_chrono-${BOOST_VERSION}")
set(boost_container_DIR "${Boost_LIB_DIR}/cmake/boost_container-${BOOST_VERSION}")
set(boost_date_time_DIR "${Boost_LIB_DIR}/cmake/boost_date_time-${BOOST_VERSION}")
set(boost_filesystem_DIR "${Boost_LIB_DIR}/cmake/boost_filesystem-${BOOST_VERSION}")
set(boost_headers_DIR "${Boost_LIB_DIR}/cmake/boost_headers-${BOOST_VERSION}")
set(boost_json_DIR "${Boost_LIB_DIR}/cmake/boost_json-${BOOST_VERSION}")
set(boost_log_DIR "${Boost_LIB_DIR}/cmake/boost_log-${BOOST_VERSION}")
set(boost_log_setup_DIR "${Boost_LIB_DIR}/cmake/boost_log_setup-${BOOST_VERSION}")
set(boost_program_options_DIR "${Boost_LIB_DIR}/cmake/boost_program_options-${BOOST_VERSION}")
set(boost_random_DIR "${Boost_LIB_DIR}/cmake/boost_random-${BOOST_VERSION}")
set(boost_regex_DIR "${Boost_LIB_DIR}/cmake/boost_regex-${BOOST_VERSION}")
set(boost_system_DIR "${Boost_LIB_DIR}/cmake/boost_system-${BOOST_VERSION}")
set(boost_thread_DIR "${Boost_LIB_DIR}/cmake/boost_thread-${BOOST_VERSION}")
set(boost_unit_test_framework_DIR "${Boost_LIB_DIR}/cmake/boost_unit_test_framework-${BOOST_VERSION}")
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_SYSTEM_PATHS ON)
option(Boost_USE_STATIC_RUNTIME "Use static runtimes" ON)

# header only libraries must not be added here
cmake_policy(SET CMP0167 NEW) # The FindBoost module is removed in favor of config files  
find_package(Boost QUIET CONFIG COMPONENTS date_time filesystem random regex system thread log log_setup program_options)
if(NOT Boost_FOUND)
    # Fallback to old FindBoost module
    cmake_policy(SET CMP0167 OLD)
    find_package(Boost QUIET COMPONENTS date_time filesystem random regex system thread log log_setup program_options)
endif()
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    message(STATUS "Found Boost: ${Boost_VERSION}")
else()
    message(STATUS "Boost not found - building without Boost support")
endif()

# --------------------------------------------------------
# SGIPC Project configuration
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_APPS "Enable application targets." FALSE)
option(BUILD_EXAMPLES "Enable demonstration targets." FALSE)
option(BUILD_DOCS "Enable documentation targets." FALSE)
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/docs" CACHE STRING "Specify doxygen output directory")

# Include project directories
include_directories(
        "${CMAKE_CURRENT_LIST_DIR}/../include"
)

# Find required packages
find_package(Threads REQUIRED)

# Generate protobuf files if protobuf is available  
file(GLOB PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/../src/proto/*.proto")
if(PROTO_FILES AND Protobuf_FOUND AND Protobuf_PROTOC_EXECUTABLE)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
    message(STATUS "Generated protobuf files from ${CMAKE_CURRENT_LIST_DIR}/../src/proto/")
else()
    message(STATUS "Protobuf not found or no proto files - creating stub header")
    # Create a stub protobuf header
    set(STUB_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ipc_messages.pb.h")
    file(WRITE "${STUB_HEADER}" 
"#ifndef IPC_MESSAGES_PB_H
#define IPC_MESSAGES_PB_H
#include <string>
#include <map>
// Stub header for when protobuf is not available
namespace sgipc { namespace proto {
    enum MessageType { UNKNOWN = 0, HEARTBEAT = 1, PORT_REQUEST = 2, PORT_RESPONSE = 3, PORT_NEGOTIATION = 4 };
    struct Heartbeat { 
        const std::string& instance_id() const { return m_instance_id; }
        void set_instance_id(const std::string& id) { m_instance_id = id; }
        const std::string& instanceId() const { return m_instance_id; }  // Alternative accessor
        uint32_t port() const { return m_port; }
        void set_port(uint32_t p) { m_port = p; }
        uint64_t timestamp() const { return m_timestamp; }
        void set_timestamp(uint64_t ts) { m_timestamp = ts; }
        const std::string& version() const { return m_version; }
        void set_version(const std::string& v) { m_version = v; }
        std::map<std::string, std::string>* mutable_metadata() { return &m_metadata; }
        const std::map<std::string, std::string>& metadata() const { return m_metadata; }
    private:
        std::string m_instance_id;
        uint32_t m_port = 0;
        uint64_t m_timestamp = 0;
        std::string m_version;
        std::map<std::string, std::string> m_metadata;
    };
    struct PortRequest { 
        std::string instance_id; 
        uint32_t preferred_port; 
        uint64_t timestamp; 
    };
    struct PortResponse { 
        std::string instance_id; 
        uint32_t assigned_port; 
        uint64_t timestamp; 
        bool success; 
        std::string error_message; 
    };
    struct IPCMessage { 
        MessageType type() const { return m_type; }
        void set_type(MessageType t) { m_type = t; }
        const std::string& sender_id() const { return m_sender_id; }
        void set_sender_id(const std::string& id) { m_sender_id = id; }
        const std::string& senderId() const { return m_sender_id; }  // Alternative accessor
        const std::string& recipient_id() const { return m_recipient_id; }
        void set_recipient_id(const std::string& id) { m_recipient_id = id; }
        uint64_t timestamp() const { return m_timestamp; }
        void set_timestamp(uint64_t ts) { m_timestamp = ts; }
        Heartbeat* mutable_heartbeat() { return &m_heartbeat; }
        const Heartbeat& heartbeat() const { return m_heartbeat; }
        bool SerializeToString(std::string* output) const { output->clear(); return true; }
        bool ParseFromString(const std::string& data) { return true; }
    private:
        MessageType m_type = UNKNOWN;
        std::string m_sender_id;
        std::string m_recipient_id;
        uint64_t m_timestamp = 0;
        Heartbeat m_heartbeat;
    };
    struct DiscoveryAnnouncement { 
        const std::string& instance_id() const { return m_instance_id; }
        void set_instance_id(const std::string& id) { m_instance_id = id; }
        const std::string& instanceId() const { return m_instance_id; }  // Alternative accessor
        const std::string& service_name() const { return m_service_name; }
        void set_service_name(const std::string& name) { m_service_name = name; }
        uint32_t port() const { return m_port; }
        void set_port(uint32_t p) { m_port = p; }
        uint64_t timestamp() const { return m_timestamp; }
        void set_timestamp(uint64_t ts) { m_timestamp = ts; }
        std::map<std::string, std::string>* mutable_capabilities() { return &m_capabilities; }
        const std::map<std::string, std::string>& capabilities() const { return m_capabilities; }
    private:
        std::string m_instance_id;
        std::string m_service_name;
        uint32_t m_port = 0;
        uint64_t m_timestamp = 0;
        std::map<std::string, std::string> m_capabilities;
    };
}}
#endif
")
    set(PROTO_HDRS "${STUB_HEADER}")
    set(PROTO_SRCS "")
endif()

# Source files
file(GLOB_RECURSE SGIPC_SOURCES 
    "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/*.mm"
)

# Remove broken libp2p source file
list(FILTER SGIPC_SOURCES EXCLUDE REGEX ".*libp2p_ipc_broken\\.cpp$")

# Create the main library
add_library(${PROJECT_NAME} STATIC
    ${SGIPC_SOURCES}
    ${PROTO_SRCS}
)

# Add compile definitions based on available features
if(Protobuf_FOUND)
    target_compile_definitions(${PROJECT_NAME} PUBLIC SGIPC_ENABLE_PROTOBUF)
endif()

# Link libraries
if(Protobuf_FOUND)
    target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} Threads::Threads)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
elseif(APPLE)
    if(IOS)
        target_link_libraries(${PROJECT_NAME} "-framework Foundation" "-framework UIKit")
    else()
        target_link_libraries(${PROJECT_NAME} "-framework Foundation" "-framework AppKit")
    endif()
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} dl)
endif()

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Export include directories for consumers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Apply installation
if(COMMAND sgipc_install)
    sgipc_install(${PROJECT_NAME})
endif()

# Build tests using addtest function (if available and enabled)
if(BUILD_TESTS AND COMMAND addtest AND GTest_FOUND)
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../test" "${CMAKE_CURRENT_BINARY_DIR}/test")
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../examples" "${CMAKE_CURRENT_BINARY_DIR}/examples")
endif()

# Install Headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" FILES_MATCHING PATTERN "*.h*")

install(TARGETS ${PROJECT_NAME} EXPORT SGIPCTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
        EXPORT SGIPCTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sgipc
        NAMESPACE sgipc::
)

include(CMakePackageConfigHelpers)

# generate the config file that is includes the exports
configure_package_config_file(${PROJECT_ROOT}/cmake/config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/SGIPCConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sgipc
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# generate the version file for the config file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/SGIPCConfigVersion.cmake"
        VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
        COMPATIBILITY AnyNewerVersion
)

# install the configuration file
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SGIPCConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sgipc
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SGIPCConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sgipc
)
