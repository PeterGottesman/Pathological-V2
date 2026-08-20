find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_filename_component(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# Generated headers land in <top-level-build-dir>/protos, matching the layout
# consuming targets already include from (both "protos/x.pb.h" via the include
# path below, and "../build/protos/x.pb.h" relative to their own source files).
set(PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/protos)

# Generates protobuf files for the scheduler_server to use
add_library(protoscheduler STATIC ${PROTO_DIR}/scheduler_server.proto)
target_link_libraries(protoscheduler gRPC::grpc++)
target_include_directories(protoscheduler PUBLIC ${CMAKE_BINARY_DIR})
protobuf_generate(TARGET protoscheduler LANGUAGE cpp IMPORT_DIRS ${PROTO_DIR} PROTOC_OUT_DIR ${PROTO_GEN_DIR})
protobuf_generate(
    TARGET protoscheduler
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${PROTO_GEN_DIR})

# Generates protobuf files for the render_server to use
add_library(protorender STATIC ${PROTO_DIR}/render_server.proto)
target_link_libraries(protorender gRPC::grpc++)
target_include_directories(protorender PUBLIC ${CMAKE_BINARY_DIR})
protobuf_generate(TARGET protorender LANGUAGE cpp IMPORT_DIRS ${PROTO_DIR} PROTOC_OUT_DIR ${PROTO_GEN_DIR})
protobuf_generate(
    TARGET protorender
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${PROTO_GEN_DIR})
