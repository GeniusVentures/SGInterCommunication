# Define the test executable using the addtest function from cmake/functions.cmake
# This function links GTest::gtest_main and GTest::gmock_main automatically
addtest(sgipc_test
        sgipc_test.cpp
)

# Link the library under test (SGInterCommunication) using the target name
target_link_libraries(sgipc_test
        ${PROJECT_NAME} # Link directly to the library target defined in CommonBuildParameters
)

# Add integration test if needed
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integration_test.cpp")
    addtest(sgipc_integration_test
            integration_test.cpp
    )
    
    target_link_libraries(sgipc_integration_test
            ${PROJECT_NAME}
    )
endif()

# Ensure GTest headers are found (should be handled by CommonBuildParameters finding GTest)
# target_include_directories(sgipc_test PRIVATE ${GTest_INCLUDE_DIR}) # Likely not needed