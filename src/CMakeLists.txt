# SGInterCommunication source library

# Source files
set(SGIPC_SOURCES 
    "${CMAKE_CURRENT_LIST_DIR}/cross_platform_ipc.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/file_based_ipc.cpp"
)

# Add libp2p source if it exists
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/libp2p_ipc.cpp")
    list(APPEND SGIPC_SOURCES "${CMAKE_CURRENT_LIST_DIR}/libp2p_ipc.cpp")
endif()

# Note: Library creation is handled in the main CMakeLists.txt
