cmake_minimum_required(VERSION 3.20)

project(pathological-sched)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
find_package(CLI11 CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Boost REQUIRED)

# Shared protoscheduler/protorender libraries, generated once from ../protos
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../protos ${CMAKE_CURRENT_BINARY_DIR}/protos-lib)

# Shared S3Manager, defined once in ../common
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_BINARY_DIR}/common-lib)

# Main executable
add_executable(pathological-sched
    src/main.cpp
    src/renderRequest.cpp
    src/requestController.cpp
    src/scheduler.cpp
    src/scheduler_server.cpp
    src/render_client.cpp
    src/render_history.cpp
)

target_link_libraries(pathological-sched PRIVATE
    CLI11::CLI11
    Drogon::Drogon
    protoscheduler
    protorender
    s3manager
    gRPC::grpc++
    Boost::boost
)
