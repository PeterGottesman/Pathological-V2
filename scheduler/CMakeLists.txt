cmake_minimum_required(VERSION 3.20)

project(pathological-sched)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
#find_package(CLI11 CONFIG REQUIRED)
#find_package(Drogon CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Generates protobuf files for the scheduler_server to use
add_library(protoscheduler protos/scheduler_server.proto)
target_link_libraries(protoscheduler gRPC::grpc++)
target_include_directories(protoscheduler PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET protoscheduler LANGUAGE cpp)
protobuf_generate(
TARGET protoscheduler
LANGUAGE grpc
GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

# Generates protobuf files for the render_server to use
add_library(protorender protos/render_server.proto)
target_link_libraries(protorender gRPC::grpc++)
target_include_directories(protorender PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET protorender LANGUAGE cpp)
protobuf_generate(
TARGET protorender
LANGUAGE grpc
GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")


# Main executable
#add_executable(pathological-sched
#    src/main.cpp
#)

#target_link_libraries(pathological-sched PRIVATE
#    CLI11::CLI11
#    Drogon::Drogon
#)

# Scheduler server executable
add_executable(scheduler-server src/scheduler_server.cpp)
target_link_libraries(scheduler-server PRIVATE protoscheduler)

# Render worker client executable
add_executable(scheduler-client src/scheduler_client.cpp)
target_link_libraries(scheduler-client PRIVATE protoscheduler)

# Render worker server executable
add_executable(render-server src/render_server.cpp)
target_link_libraries(render-server PRIVATE protorender)

# Scheduler client executable
add_executable(render-client src/render_client.cpp)
target_link_libraries(render-client PRIVATE protorender)
