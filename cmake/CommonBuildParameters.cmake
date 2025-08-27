# PROTOBUF VERSION TO USE
set(PROTOBUF_MAJOR_VERSION "3" CACHE STRING "Protobuf Major Version")
set(PROTOBUF_MINOR_VERSION "21" CACHE STRING "Protobuf Minor Version")
set(PROTOBUF_PATCH_VERSION "0" CACHE STRING "Protobuf Patch Version")

# convenience settings
set(PROTOBUF_VERSION "${PROTOBUF_MAJOR_VERSION}.${PROTOBUF_MINOR_VERSION}.${PROTOBUF_PATCH_VERSION}")
set(PROTOBUF_VERSION_2U "${PROTOBUF_MAJOR_VERSION}_${PROTOBUF_MINOR_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------------
# Set config for Boost.Outcome using GeniusVentures thirdparty
set(BOOST_ROOT "${_THIRDPARTY_BUILD_DIR}/boost")
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS ON)

# Find Boost.Outcome (header-only library)
find_package(Boost QUIET COMPONENTS outcome)
if(NOT Boost_FOUND)
    # Fallback: try to find Boost in system
    find_package(Boost QUIET)
endif()

if(DEFINED USE_PROTOBUF_INCLUDE_POSTFIX)
    set(PROTOBUF_INCLUDE_POSTFIX "/protobuf-${PROTOBUF_VERSION_2U}" CACHE STRING "Protobuf include postfix")
endif()

# --------------------------------------------------------
# Set config of GTest using GeniusVentures thirdparty
set(GTest_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/lib/cmake/GTest")
set(GTest_INCLUDE_DIR "${_THIRDPARTY_BUILD_DIR}/GTest/include")

# Try to find GTest in thirdparty, fall back to system if not found
find_package(GTest QUIET PATHS ${GTest_DIR} NO_DEFAULT_PATH)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

if(GTest_FOUND)
    message(STATUS "Found GTest version: ${GTest_VERSION}")
    if(GTest_INCLUDE_DIR AND EXISTS ${GTest_INCLUDE_DIR})
        include_directories(${GTest_INCLUDE_DIR})
    endif()
else()
    message(WARNING "GTest not found in thirdparty or system. Tests will be disabled.")
    set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
endif()

# Protobuf should be loaded before other dependencies
# --------------------------------------------------------
# Set config of Protobuf project using GeniusVentures thirdparty
# Check if protobuf exists in thirdparty (leave it out as requested if missing)
if(EXISTS "${_THIRDPARTY_BUILD_DIR}/grpc")
    # Protobuf is usually included with grpc in the thirdparty repo
    set(_PROTOBUF_ROOT "${_THIRDPARTY_BUILD_DIR}/grpc/build")
    set(Protobuf_LIB_DIR "${_PROTOBUF_ROOT}/lib")
    set(Protobuf_INCLUDE_DIR "${_PROTOBUF_ROOT}/include")
    set(Protobuf_DIR "${Protobuf_LIB_DIR}/cmake/protobuf")
    set(protobuf_DIR "${Protobuf_LIB_DIR}/cmake/protobuf")
    set(Protobuf_USE_STATIC_LIBS ON)
    set(Protobuf_NO_SYSTEM_PATHS ON)
    
    find_package(Protobuf QUIET PATHS ${Protobuf_DIR} NO_DEFAULT_PATH)
    if(Protobuf_FOUND)
        message(STATUS "Found Protobuf in thirdparty: ${Protobuf_VERSION}")
        include_directories(${Protobuf_INCLUDE_DIRS})
    endif()
endif()

# If protobuf not found in thirdparty, leave it out as requested
if(NOT Protobuf_FOUND)
    message(STATUS "Protobuf not found in thirdparty - skipping as requested")
    set(SGIPC_MINIMAL_BUILD ON CACHE BOOL "Use minimal build without protobuf" FORCE)
endif()

# --------------------------------------------------------
# set config for sgipc
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_APPS "Enable application targets." FALSE)
option(BUILD_EXAMPLES "Enable demonstration targets." FALSE)
option(BUILD_DOCS "Enable documentation targets." FALSE)
set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/docs" CACHE STRING "Specify doxygen output directory")

include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/../include"
)

# Only include src CMakeLists if it exists
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../src/CMakeLists.txt")
    include("${CMAKE_CURRENT_LIST_DIR}/../src/CMakeLists.txt")
endif()

if(BUILD_TESTS AND GTest_FOUND)
    add_executable(${PROJECT_NAME}_test
        "${CMAKE_CURRENT_LIST_DIR}/../test/sgipc_test.cpp"
    )
    target_link_libraries(${PROJECT_NAME}_test PUBLIC ${PROJECT_NAME} GTest::gtest)
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
