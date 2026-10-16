cmake_minimum_required(VERSION 3.16)
project(secure_ota_host_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SECURE_OTA_SANITIZE "Build the tests with AddressSanitizer and UBSan" OFF)

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
    set(googletest_system SYSTEM)
endif()
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    ${googletest_system}
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(component_dir ${CMAKE_CURRENT_SOURCE_DIR}/../../components/secure_ota)

add_executable(secure_ota_tests
    version_test.cpp
    ${component_dir}/secure_ota_version.c
)
target_include_directories(secure_ota_tests PRIVATE ${component_dir}/include)
target_link_libraries(secure_ota_tests PRIVATE GTest::gtest_main)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(secure_ota_tests PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
    if(SECURE_OTA_SANITIZE)
        target_compile_options(secure_ota_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(secure_ota_tests PRIVATE -fsanitize=address,undefined)
    endif()
endif()

enable_testing()
include(GoogleTest)
gtest_discover_tests(secure_ota_tests)
