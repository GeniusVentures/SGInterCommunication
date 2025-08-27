cmake_minimum_required(VERSION 3.16)

# Simple IPC example - works with both minimal and full build
add_executable(simple_ipc_example
    simple_ipc_example.cpp
)

target_link_libraries(simple_ipc_example
    sgipc
)

target_include_directories(simple_ipc_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

# TODO: Fix other examples to work with minimal build
# The multi_instance_example.cpp and port_negotiation_example.cpp need updates
# to use the correct PascalCase function names and conditional compilation
