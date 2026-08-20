cmake_minimum_required(VERSION 3.20)

project(pathological-v2)

option(BUILD_SCHEDULER "Build the scheduler" ON)
option(BUILD_RENDER_WORKER "Build the render worker" ON)

if(NOT BUILD_SCHEDULER AND NOT BUILD_RENDER_WORKER)
    message(FATAL_ERROR "At least one of BUILD_SCHEDULER or BUILD_RENDER_WORKER must be ON")
endif()

# protoscheduler/protorender/s3manager are shared by both components, so
# they're defined once here rather than by whichever subdirectory needs
# them first.
add_subdirectory(protos)
add_subdirectory(common)

if(BUILD_SCHEDULER)
    add_subdirectory(scheduler)
endif()

if(BUILD_RENDER_WORKER)
    add_subdirectory(render_worker)
endif()
