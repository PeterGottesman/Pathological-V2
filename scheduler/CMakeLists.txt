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

# Generates protobuf files for the scheduler_server to use
add_library(protolib protos/scheduler_server.proto)
target_link_libraries(protolib gRPC::grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET protolib LANGUAGE cpp)
protobuf_generate(
TARGET protolib
LANGUAGE grpc
GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")


# Main executable
add_executable(pathological-sched
    src/main.cpp
)

target_link_libraries(pathological-sched PRIVATE
    CLI11::CLI11
    Drogon::Drogon
)

# Scheduler gRPC server executable
add_executable(scheduler-server
    src/scheduler_server.cpp
)

target_link_libraries(scheduler-server PRIVATE
    protolib
)
