# Test configuration for SGInterCommunication
# Uses RLP-style addtest function

# Test executable using RLP-style function
addtest(sgipc_test
    sgipc_test.cpp
    integration_test.cpp
)

target_link_libraries(sgipc_test
    sgipc
    ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories
target_include_directories(sgipc_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
