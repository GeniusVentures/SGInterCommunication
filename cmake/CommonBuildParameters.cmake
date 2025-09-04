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


# Set the protoc executable path for protobuf generation
set(PROTOC_EXECUTABLE "${_THIRDPARTY_BUILD_DIR}/grpc/bin/protoc${CMAKE_EXECUTABLE_SUFFIX}")
set(Protobuf_PROTOC_EXECUTABLE ${PROTOC_EXECUTABLE} CACHE PATH "Initial cache" FORCE)

# --------------------------------------------------------
# Set config of GTest (Optional)
set(GTest_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/lib/cmake/GTest")
set(GTest_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/include")
find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
    include_directories(${GTest_INCLUDE_DIR})
endif()

# --------------------------------------------------------
# Set config of Protobuf (Required)
set(Protobuf_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/cmake")
set(Protobuf_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/include")
set(Protobuf_LIBRARY_DIR "${_THIRDPARTY_BUILD_DIR}/grpc/lib")
set(Protobuf_PROTOC_EXECUTABLE "${_THIRDPARTY_BUILD_DIR}/grpc/bin/protoc.exe")

# Force CMake to use our thirdparty protobuf
set(CMAKE_PREFIX_PATH "${_THIRDPARTY_BUILD_DIR}/grpc" ${CMAKE_PREFIX_PATH})
set(Protobuf_ROOT "${_THIRDPARTY_BUILD_DIR}/grpc")

# Clear any existing protobuf targets before loading
if(TARGET protobuf::protoc)
    set_target_properties(protobuf::protoc PROPERTIES IMPORTED FALSE)
    unset(protobuf::protoc)
endif()

# Use thirdparty protobuf exclusively
find_package(Protobuf REQUIRED PATHS "${_THIRDPARTY_BUILD_DIR}/grpc" NO_DEFAULT_PATH)
message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
message(STATUS "Protobuf DIR: ${Protobuf_DIR}")
message(STATUS "Protobuf Libraries: ${Protobuf_LIBRARIES}")

# Include protobuf generate functionality
include("${Protobuf_DIR}/protobuf-generate.cmake")
include_directories(${Protobuf_INCLUDE_DIR})

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
# Set config of libp2p (Required for Libp2pIPC implementation)
set(Libp2p_DIR "${_THIRDPARTY_BUILD_DIR}/libp2p/lib/cmake")
set(Libp2p_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/libp2p/include")
set(Libp2p_LIBRARY_DIR "${_THIRDPARTY_BUILD_DIR}/libp2p/lib")

# Check if libp2p is available
if(EXISTS "${Libp2p_INCLUDE_DIR}/libp2p/libp2p.hpp")
    set(LIBP2P_FOUND TRUE)
    message(STATUS "Found libp2p at: ${Libp2p_INCLUDE_DIR}")
    
    # Include libp2p headers
    include_directories("${Libp2p_INCLUDE_DIR}")
    
    # libp2p depends on soralog, include it as well
    set(Soralog_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/soralog/include")
    if(EXISTS "${Soralog_INCLUDE_DIR}")
        include_directories("${Soralog_INCLUDE_DIR}")
        message(STATUS "Found soralog at: ${Soralog_INCLUDE_DIR}")
    endif()
    
    # libp2p is always available, no need for conditional compilation
else()
    message(FATAL_ERROR "libp2p not found at expected location: ${Libp2p_INCLUDE_DIR}")
endif()

# --------------------------------------------------------
# SGIPC Project configuration
option(BUILD_TESTS "Build tests" OFF)  # Temporarily disabled
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_APPS "Enable application targets." FALSE)
option(BUILD_EXAMPLES "Enable demonstration targets." TRUE)  # Enable examples
option(BUILD_DOCS "Enable documentation targets." FALSE)
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/docs" CACHE STRING "Specify doxygen output directory")

# Include project directories
include_directories(
        "${CMAKE_CURRENT_LIST_DIR}/../include"
)

# Find required packages
find_package(Threads REQUIRED)

# Generate protobuf files
file(GLOB PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/../src/proto/*.proto")
if(PROTO_FILES)
    # Use modern protobuf generation with explicit import paths
    protobuf_generate(
        LANGUAGE cpp
        OUT_VAR PROTO_GENERATED_FILES
        IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../src/proto" "${Protobuf_INCLUDE_DIR}"
        PROTOS ${PROTO_FILES}
    )
    
    # Extract the generated .cc and .h files
    set(PROTO_SRCS "")
    set(PROTO_HDRS "")
    foreach(PROTO_GEN_FILE ${PROTO_GENERATED_FILES})
        if(PROTO_GEN_FILE MATCHES "\\.cc$")
            list(APPEND PROTO_SRCS ${PROTO_GEN_FILE})
        elseif(PROTO_GEN_FILE MATCHES "\\.h$")
            list(APPEND PROTO_HDRS ${PROTO_GEN_FILE})
        endif()
    endforeach()
    
    message(STATUS "Generated protobuf files from ${CMAKE_CURRENT_LIST_DIR}/../src/proto/")
else()
    message(STATUS "No proto files found")
    set(PROTO_HDRS "")
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

# Set C++ standard and runtime library for Windows
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
    )
endif()

# Add compile definitions and link libraries
target_compile_definitions(${PROJECT_NAME} PUBLIC SGIPC_ENABLE_PROTOBUF)

# Link protobuf libraries from thirdparty directory
if(WIN32)
    # Link protobuf library file directly from thirdparty on Windows
    set(PROTOBUF_LIBRARY_FILE "${Protobuf_LIBRARY_DIR}/libprotobuf.lib")
    if(EXISTS "${PROTOBUF_LIBRARY_FILE}")
        target_link_libraries(${PROJECT_NAME} "${PROTOBUF_LIBRARY_FILE}")
        message(STATUS "Linking protobuf from: ${PROTOBUF_LIBRARY_FILE}")
    else()
        message(WARNING "Protobuf library not found at: ${PROTOBUF_LIBRARY_FILE}")
        # Fallback to found protobuf libraries
        target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARIES})
    endif()
else()
    target_link_libraries(${PROJECT_NAME} ${Protobuf_LIBRARIES})
endif()

# Link essential abseil and utf8_range libraries from thirdparty directory
set(THIRDPARTY_ABSEIL_LIBS
    absl_base
    absl_strings 
    absl_string_view
    absl_str_format_internal
    absl_strings_internal
    absl_synchronization
    absl_time
    absl_time_zone
    absl_int128
    absl_hash
    absl_city
    absl_low_level_hash
    absl_raw_hash_set
    absl_hashtablez_sampler
    absl_cord
    absl_cordz_functions
    absl_cordz_handle
    absl_cordz_info
    absl_cord_internal
    absl_status
    absl_statusor
    absl_raw_logging_internal
    absl_throw_delegate
    absl_debugging_internal
    absl_stacktrace
    absl_symbolize
    absl_demangle_internal
    absl_malloc_internal
    absl_spinlock_wait
    absl_strerror
    absl_crc_cord_state
    absl_crc_cpu_detect
    absl_crc_internal
    absl_crc32c
    absl_log_entry
    absl_log_flags
    absl_log_globals
    absl_log_initialize
    absl_log_sink
    absl_log_internal_check_op
    absl_log_internal_conditions
    absl_log_internal_format
    absl_log_internal_globals
    absl_log_internal_log_sink_set
    absl_log_internal_message
    absl_log_internal_nullguard
    absl_log_internal_proto
    absl_log_severity
    absl_graphcycles_internal
    absl_kernel_timeout_internal
    absl_examine_stack
    absl_failure_signal_handler
    absl_periodic_sampler
    absl_poison
    absl_flags_commandlineflag
    absl_flags_commandlineflag_internal
    absl_flags_config
    absl_flags_internal
    absl_flags_marshalling
    absl_flags_parse
    absl_flags_private_handle_accessor
    absl_flags_program_name
    absl_flags_reflection
    absl_flags_usage
    absl_flags_usage_internal
    absl_leak_check
    absl_random_distributions
    absl_random_internal_distribution_test_util
    absl_random_internal_platform
    absl_random_internal_pool_urbg
    absl_random_internal_randen
    absl_random_internal_randen_hwaes
    absl_random_internal_randen_hwaes_impl
    absl_random_internal_randen_slow
    absl_random_internal_seed_material
    absl_random_seed_gen_exception
    absl_random_seed_sequences
    absl_scoped_set_env
    absl_utf8_for_code_point
    absl_vlog_config_internal
    utf8_range_lib
    utf8_validity
)

foreach(ABSL_LIB ${THIRDPARTY_ABSEIL_LIBS})
    if(EXISTS "${Protobuf_LIBRARY_DIR}/${ABSL_LIB}.lib")
        target_link_libraries(${PROJECT_NAME} "${Protobuf_LIBRARY_DIR}/${ABSL_LIB}.lib")
    endif()
endforeach()

# Link libp2p libraries if available
if(LIBP2P_FOUND)
    set(LIBP2P_CORE_LIBS
        p2p
        p2p_basic_host
        p2p_default_host
        p2p_default_network
        p2p_gossip
        p2p_identify
        p2p_kademlia
        p2p_autonat
        p2p_ping
        p2p_tcp
        p2p_tcp_connection
        p2p_tcp_listener
        p2p_multiselect
        p2p_multiaddress
        p2p_peer_id
        p2p_crypto_provider
        p2p_peer_repository
        p2p_connection_manager
        p2p_dialer
        p2p_listener_manager
        p2p_transport_manager
        p2p_upgrader
        p2p_noise
        p2p_yamux
        p2p_yamuxed_connection
        p2p_mplex
        p2p_mplexed_connection
        p2p_scheduler
        p2p_asio_scheduler_backend
        asio_scheduler
    )
    
    foreach(LIB ${LIBP2P_CORE_LIBS})
        if(EXISTS "${Libp2p_LIBRARY_DIR}/${LIB}.lib")
            target_link_libraries(${PROJECT_NAME} "${Libp2p_LIBRARY_DIR}/${LIB}.lib")
            message(STATUS "Linking libp2p library: ${LIB}")
        endif()
    endforeach()
    
    message(STATUS "libp2p libraries linked successfully")
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
