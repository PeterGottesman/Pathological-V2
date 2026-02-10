cmake_minimum_required(VERSION 3.20)

project(pathological-sched)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
find_package(CLI11 CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)

# Main executable
add_executable(pathological-sched
    src/main.cpp
)

target_link_libraries(pathological-sched PRIVATE
    CLI11::CLI11
    Drogon::Drogon
)
