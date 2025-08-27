cmake_minimum_required(VERSION 3.16)

# Simple IPC example
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

# Multi-instance example
add_executable(multi_instance_example
    multi_instance_example.cpp
)

target_link_libraries(multi_instance_example
    sgipc
)

target_include_directories(multi_instance_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

# Port negotiation example
add_executable(port_negotiation_example
    port_negotiation_example.cpp
)

target_link_libraries(port_negotiation_example
    sgipc
)

target_include_directories(port_negotiation_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
