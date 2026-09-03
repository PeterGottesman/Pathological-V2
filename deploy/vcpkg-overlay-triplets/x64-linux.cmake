# Overlay for vcpkg's stock x64-linux triplet, used only by the Docker
# builds (see deploy/docker/*.Dockerfile) -- adds VCPKG_BUILD_TYPE=release
# so vcpkg only compiles the release variant of each dependency instead of
# both debug and release. A deployed container never needs the debug
# artifacts, and skipping them roughly halves dependency build time.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_BUILD_TYPE release)
