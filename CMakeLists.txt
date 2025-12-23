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

# Main executable
add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
    src/scene.cpp
)

target_link_libraries(pathological PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    CLI11::CLI11
)

target_include_directories(pathological PRIVATE ${Stb_INCLUDE_DIR})

# Shader compilation
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/raygen.rgen
    ${SHADER_SOURCE_DIR}/miss.rmiss
    ${SHADER_SOURCE_DIR}/closesthit.rchit
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND Vulkan::glslc ${SHADER} -o ${SHADER_OUTPUT} --target-env=vulkan1.3
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPV_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_dependencies(pathological shaders)

# Copy shaders to output directory for runtime access
add_custom_command(TARGET pathological POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_BINARY_DIR} $<TARGET_FILE_DIR:pathological>/shaders
)
