cmake_minimum_required(VERSION 3.16)

# Simple IPC example - works with both minimal and full build
add_executable(simple_ipc_example
    simple_ipc_example.cpp
)

# Set C++ standard and runtime library to match the dependencies
set_target_properties(simple_ipc_example PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Ensure static runtime on Windows to match protobuf
if(WIN32)
    set_target_properties(simple_ipc_example PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

target_link_libraries(simple_ipc_example
    SGInterCommunication
)

# Platform-specific libraries needed by protobuf
if(WIN32)
    target_link_libraries(simple_ipc_example 
        ws2_32 
        wsock32
        userenv
        crypt32
    )
elseif(APPLE)
    if(IOS)
        target_link_libraries(simple_ipc_example "-framework Foundation" "-framework UIKit")
    else()
        target_link_libraries(simple_ipc_example "-framework Foundation" "-framework AppKit")
    endif()
elseif(UNIX AND NOT APPLE)
    target_link_libraries(simple_ipc_example dl)
endif()

target_include_directories(simple_ipc_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

# TODO: Fix other examples to work with minimal build
# The multi_instance_example.cpp and port_negotiation_example.cpp need updates
# to use the correct PascalCase function names and conditional compilation
