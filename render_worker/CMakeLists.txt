cmake_minimum_required(VERSION 3.20)

project(pathological)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS uuid)


# tinygltf is header-only
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

# protoscheduler, protorender, and s3manager are defined by the top-level
# CMakeLists.txt before this subdirectory is added.

# Main executable
add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
    src/scene.cpp
    src/scene_builder.cpp
    src/scene_graph.cpp
    src/path_tracer.cpp
    src/render_worker.cpp
    src/render_server.cpp
    src/scheduler_client.cpp
    src/job.cpp
    src/render_jobs.cpp
)

target_link_libraries(pathological PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    CLI11::CLI11
    protorender
    protoscheduler
    s3manager
    Boost::uuid
)

target_include_directories(pathological PRIVATE ${Stb_INCLUDE_DIR} ${TINYGLTF_INCLUDE_DIRS})

# Shader compilation
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/raygen.rgen
    ${SHADER_SOURCE_DIR}/miss.rmiss
    ${SHADER_SOURCE_DIR}/closesthit.rchit
)

set(SHADER_INCLUDES
    ${SHADER_SOURCE_DIR}/common.glsl
)

foreach(shader ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${shader} NAME)
    set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND Vulkan::glslc -I${SHADER_SOURCE_DIR} ${shader} -o ${SHADER_OUTPUT} --target-env=vulkan1.3
        DEPENDS ${shader} ${SHADER_INCLUDES}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPV_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS} COMMENT "Building shaders")
add_dependencies(pathological shaders)

# Copy shaders to output directory for runtime access
add_custom_command(TARGET pathological POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_BINARY_DIR} $<TARGET_FILE_DIR:pathological>/shaders
    COMMENT "Copying shaders to output directory"
)
