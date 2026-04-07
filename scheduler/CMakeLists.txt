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
find_package(AWSSDK REQUIRED COMPONENTS s3)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_filename_component(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../protos ABSOLUTE)

# Generates protobuf files for the scheduler_server to use
add_library(protoscheduler STATIC ${PROTO_DIR}/scheduler_server.proto)
target_link_libraries(protoscheduler gRPC::grpc++)
target_include_directories(protoscheduler PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET protoscheduler LANGUAGE cpp IMPORT_DIRS ${PROTO_DIR} PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)
protobuf_generate(
    TARGET protoscheduler
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)

# Generates protobuf files for the render_server to use
add_library(protorender STATIC ${PROTO_DIR}/render_server.proto)
target_link_libraries(protorender gRPC::grpc++)
target_include_directories(protorender PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET protorender LANGUAGE cpp IMPORT_DIRS ${PROTO_DIR} PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)
protobuf_generate(
    TARGET protorender
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)

# Main executable
add_executable(pathological-sched
    src/main.cpp
    src/renderRequest.cpp
    src/requestController.cpp
    src/scheduler.cpp
    src/scheduler_server.cpp
    src/render_client.cpp
    src/s3Manager.cpp
)

target_link_libraries(pathological-sched PRIVATE
    CLI11::CLI11
    Drogon::Drogon
    protoscheduler
    protorender
    ${AWSSDK_LINK_LIBRARIES}
    gRPC::grpc++
)
