cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS Y)

project(pathological)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(pathological src/main.cpp)

target_link_libraries(pathological PRIVATE glfw glm::glm Vulkan::Vulkan)
