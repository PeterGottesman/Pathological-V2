# Overlay for vcpkg's stock arm64-linux triplet (Jetson). Same rationale as
# x64-linux.cmake: release-only dependencies, which matters even more on
# embedded hardware where compiling gRPC twice is very slow.
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_BUILD_TYPE release)
