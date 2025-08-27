cmake_minimum_required(VERSION 3.16)

project(SGInterCommunication 
    VERSION 1.0.0
    DESCRIPTION "Cross-Platform IPC Service for GeniusSDK"
    LANGUAGES CXX)

# Set up thirdparty path early
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Find Threads early as it's needed for both minimal and full builds
find_package(Threads REQUIRED)

# Include custom cmake modules first
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Try to include RLP-style modules but make them optional
include(functions OPTIONAL)

# Set C++ standard directly if CommonBuildParameters fails
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Try to include CommonBuildParameters, but continue if it fails
if(NOT SGIPC_MINIMAL_BUILD)
    include(CommonBuildParameters OPTIONAL RESULT_VARIABLE CB_FOUND)
    if(NOT CB_FOUND)
        message(WARNING "CommonBuildParameters not found, using minimal configuration")
        set(SGIPC_MINIMAL_BUILD ON)
    endif()
else()
    message(STATUS "Using minimal build configuration")
endif()

print("Starting SGInterCommunication build configuration")
print("Project version: ${PROJECT_VERSION}")

# Build options
option(BUILD_TESTS "Build test programs" OFF)  # Default to OFF until GTest is available
option(BUILD_EXAMPLES "Build example programs" ON)
option(SGIPC_MINIMAL_BUILD "Build minimal version without external dependencies" ON)

if(SGIPC_MINIMAL_BUILD)
    print("Building minimal version without external dependencies")
    
    # Include directories
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # For minimal build, create a simple message header instead of protobuf
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/simple_messages.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/sgipc/simple_messages.hpp"
        @ONLY
    )
    
    # Source files (exclude protobuf-dependent files in minimal build)
    set(SGIPC_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/cross_platform_ipc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/file_based_ipc.cpp"
    )
    
    # Add platform-specific sources only if they exist
    if(WIN32)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_ipc.cpp")
            list(APPEND SGIPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_ipc.cpp")
        endif()
    elseif(APPLE)
        if(IOS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ios_pasteboard_ipc.mm")
            list(APPEND SGIPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/ios_pasteboard_ipc.mm")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/macos_ipc.cpp")
            list(APPEND SGIPC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/macos_ipc.cpp")
        endif()
    endif()
    
    # Create the main library
    add_library(sgipc STATIC ${SGIPC_SOURCES})
    
    # Add preprocessor definition for minimal build
    target_compile_definitions(sgipc PUBLIC SGIPC_MINIMAL_BUILD)
    
    # Apply RLP-style installation if function is available
    if(COMMAND sgipc_install)
        sgipc_install(sgipc)
    else()
        # Fallback installation
        install(TARGETS sgipc
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
        )
    endif()
    
    # Platform-specific libraries
    if(WIN32)
        target_link_libraries(sgipc ws2_32 wsock32)
    elseif(APPLE)
        if(IOS)
            target_link_libraries(sgipc "-framework Foundation" "-framework UIKit")
        else()
            target_link_libraries(sgipc "-framework Foundation" "-framework AppKit")
        endif()
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(sgipc dl)
    endif()
    
    # Link Threads
    target_link_libraries(sgipc Threads::Threads)
    
    # Set target properties
    set_target_properties(sgipc PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    
    # Export include directories for consumers
    target_include_directories(sgipc PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    
else()
    # Full build with all dependencies
    print("Building full version with external dependencies")
    
    # Set up libp2p from thirdparty if available
    set(LIBP2P_DIR "${THIRDPARTY_DIR}/libp2p")
    if(EXISTS "${LIBP2P_DIR}")
        print("Found libp2p in thirdparty directory")
        # Add libp2p subdirectory if it exists and has CMakeLists.txt
        if(EXISTS "${LIBP2P_DIR}/CMakeLists.txt")
            add_subdirectory(${LIBP2P_DIR} ${CMAKE_BINARY_DIR}/libp2p EXCLUDE_FROM_ALL)
        endif()
        set(ENABLE_LIBP2P ON)
    else()
        print("libp2p not found in thirdparty - disabling libp2p backend")
        set(ENABLE_LIBP2P OFF)
    endif()
    
    # Generate protobuf files if protobuf is available
    file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
    if(PROTO_FILES AND Protobuf_FOUND)
        protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
        print("Generated protobuf files from ${CMAKE_CURRENT_SOURCE_DIR}/proto/")
    endif()
    
    # Include directories
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Source files
    file(GLOB_RECURSE SGIPC_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
    )
    
    # Remove libp2p source if libp2p is not available
    if(NOT ENABLE_LIBP2P)
        list(FILTER SGIPC_SOURCES EXCLUDE REGEX ".*libp2p.*")
    endif()
    
    # Create the main library
    add_library(sgipc STATIC
        ${SGIPC_SOURCES}
        ${PROTO_SRCS}
    )
    
    # Add compile definitions based on available features
    if(ENABLE_LIBP2P)
        target_compile_definitions(sgipc PUBLIC SGIPC_ENABLE_LIBP2P)
    endif()
    
    if(Protobuf_FOUND)
        target_compile_definitions(sgipc PUBLIC SGIPC_ENABLE_PROTOBUF)
    endif()
    
    # Apply RLP-style installation if function is available
    if(COMMAND sgipc_install)
        sgipc_install(sgipc)
    endif()
    
    # Link libraries
    if(Protobuf_FOUND)
        target_link_libraries(sgipc ${Protobuf_LIBRARIES})
    endif()
    
    if(ENABLE_LIBP2P)
        # Link libp2p if available
        target_link_libraries(sgipc libp2p)
    endif()
    
    target_link_libraries(sgipc Threads::Threads)
    
    # Platform-specific libraries
    if(WIN32)
        target_link_libraries(sgipc ws2_32 wsock32)
    elseif(APPLE)
        if(IOS)
            target_link_libraries(sgipc "-framework Foundation" "-framework UIKit")
        else()
            target_link_libraries(sgipc "-framework Foundation" "-framework AppKit")
        endif()
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(sgipc dl)
    endif()
    
    # Set target properties
    set_target_properties(sgipc PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    
    # Export include directories for consumers
    target_include_directories(sgipc PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
    )
endif()

# Build tests using RLP-style addtest function (if available and enabled)
if(BUILD_TESTS AND COMMAND addtest)
    enable_testing()
    add_subdirectory(test)
endif()

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

print("Configuration complete. Building SGIPC library with tests=${BUILD_TESTS}, examples=${BUILD_EXAMPLES}, minimal=${SGIPC_MINIMAL_BUILD}")
