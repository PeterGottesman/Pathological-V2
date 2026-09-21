# Overlay for vcpkg's stock x64-linux triplet. Adds VCPKG_BUILD_TYPE=release
# so vcpkg only compiles the release variant of each dependency instead of
# both debug and release, roughly halving dependency build time and shrinking
# vcpkg_installed from ~4 GB to ~150 MB. Our own code still builds in Debug
# against release-only dependencies; only debug symbols inside the
# dependencies are lost.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_BUILD_TYPE release)
