# SGInterCommunication source library

# Source files for minimal build
set(SGIPC_SOURCES 
    "${CMAKE_CURRENT_LIST_DIR}/cross_platform_ipc.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/file_based_ipc.cpp"
)

# Don't include libp2p implementation in minimal build
# Add libp2p source if it exists (for minimal build without dependencies)
# if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/libp2p_ipc.cpp")
#     list(APPEND SGIPC_SOURCES "${CMAKE_CURRENT_LIST_DIR}/libp2p_ipc.cpp")
# endif()

# Create the main library
add_library(${PROJECT_NAME} STATIC ${SGIPC_SOURCES})

# Add preprocessor definition for minimal build
target_compile_definitions(${PROJECT_NAME} PUBLIC SGIPC_MINIMAL_BUILD)

# Export include directories for consumers
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

# For minimal build, create a simple message header instead of protobuf
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/../cmake/simple_messages.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/sgipc/simple_messages.hpp"
    @ONLY
)

# Find Threads as it's needed
find_package(Threads REQUIRED)

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

# Link Threads
target_link_libraries(${PROJECT_NAME} Threads::Threads)

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
