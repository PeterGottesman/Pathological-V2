find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
get_filename_component(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# Generated headers land under this subdirectory's own binary dir (unique
# regardless of how many consumers add_subdirectory() this), and get exposed
# to consumers as "protos/x.pb.h" via the PUBLIC include directory below.
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/protos)

# Generates protobuf files for the scheduler_server to use
add_library(protoscheduler STATIC ${PROTO_DIR}/scheduler_server.proto)
target_link_libraries(protoscheduler gRPC::grpc++)
target_include_directories(protoscheduler PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
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
target_include_directories(protorender PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET protorender LANGUAGE cpp IMPORT_DIRS ${PROTO_DIR} PROTOC_OUT_DIR ${PROTO_GEN_DIR})
protobuf_generate(
    TARGET protorender
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    IMPORT_DIRS ${PROTO_DIR}
    PROTOC_OUT_DIR ${PROTO_GEN_DIR})
