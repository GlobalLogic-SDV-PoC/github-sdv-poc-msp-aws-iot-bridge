cmake_minimum_required(VERSION 3.5.2)
project(
    aws_iot_bridge_core
    LANGUAGES CXX
    VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(AWS_IOT_BRIDGE_ENABLE_TESTING "Specify if testing should be enabled" OFF)
option(AWS_IOT_BRIDGE_ENABLE_CODE_COVERAGE
       "Specify if code coverage should be enabled" OFF)
option(AWS_IOT_BRIDGE_EXTERNAL_GTEST
       "Specify whether to use an external googletest library" OFF)
set(LOG_LEVELS
    TRACE
    DEBUG
    INFO
    WARN
    ERROR
    CRITICAL
    OFF)

set(AWS_IOT_BRIDGE_LOG_LEVEL
    "TRACE"
    CACHE STRING "Specify spdlog SPDLOG_ACTIVE_LEVEL ${LOG_LEVELS}")

if(AWS_IOT_BRIDGE_ENABLE_TESTING)
    if(AWS_IOT_BRIDGE_ENABLE_CODE_COVERAGE)
        include(CodeCoverage)
        append_coverage_compiler_flags()
    endif()

    if(NOT AWS_IOT_BRIDGE_EXTERNAL_GTEST)
        add_subdirectory(dep/external/gtest)
    endif()
endif()

add_subdirectory(dep/internal/iot)
set(BUILD_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(ENABLE_MQTT5_TEST
    OFF
    CACHE BOOL "" FORCE)
set(ENABLE_NET_TESTS
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(dep/external/aws_crt)

add_library(${PROJECT_NAME} STATIC src/client_iot.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC iot_bridge_core spdlog::spdlog
    PRIVATE aws-crt-cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

list(FIND LOG_LEVELS ${AWS_IOT_BRIDGE_LOG_LEVEL} LOG_LEVEL_INDEX)

if(LOG_LEVEL_INDEX EQUAL -1)
    message(
        FATAL_ERROR
            "Invalid LOG_LEVEL: ${AWS_IOT_BRIDGE_LOG_LEVEL}. Allowed values are ${LOG_LEVELS}."
    )
endif()

message("Setting LOG_LEVEL to ${AWS_IOT_BRIDGE_LOG_LEVEL}")
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE SPDLOG_ACTIVE_LEVEL=${LOG_LEVEL_INDEX})

if(AWS_IOT_BRIDGE_ENABLE_TESTING)
    add_subdirectory(test)
endif()
