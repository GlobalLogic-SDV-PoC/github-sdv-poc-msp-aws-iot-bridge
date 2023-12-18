cmake_minimum_required(VERSION 3.10)
project(aws_iot)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(iotb REQUIRED)

set(BUILD_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(ENABLE_NET_TESTS
    OFF
    CACHE BOOL "" FORCE)
add_subdirectory(dep/external/aws)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(${PROJECT_NAME} src/main.cpp src/client_iot.cpp
                               src/client_iot_impl.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
ament_target_dependencies(${PROJECT_NAME} rclcpp std_msgs iotb)
target_link_libraries(${PROJECT_NAME} aws-crt-cpp)

install(TARGETS ${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

ament_package()
