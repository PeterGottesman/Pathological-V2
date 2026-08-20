cmake_minimum_required(VERSION 3.20)

project(pathological-v2)

# scheduler must come first: it defines the shared protoscheduler/protorender
# and s3manager targets (via ../protos and ../common) that render_worker
# reuses instead of redefining.
add_subdirectory(scheduler)
add_subdirectory(render_worker)
