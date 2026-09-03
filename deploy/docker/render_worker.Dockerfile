# syntax=docker/dockerfile:1
# Build context is the repo root, e.g.:
#   docker build -f deploy/docker/render_worker.Dockerfile -t pathological-render-worker .
#
# Runs on Mesa's lavapipe (llvmpipe) software Vulkan driver by default, which
# supports the hardware ray-tracing extensions this app requires (Mesa 24+).
# That's plenty for local testing; on the real GPU cluster, swap the runtime
# base/ICD for one that exposes the real hardware driver (see deploy/README.md).

FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl zip unzip tar pkg-config \
        python3 ca-certificates libvulkan-dev glslc \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg

# One of this target's ports needs a newer CMake than Ubuntu 24.04 ships
# (3.28). Left alone, vcpkg downloads its own private CMake mid-build to
# satisfy that -- which, in this environment, corrupts the outer configure's
# compiler/generator detection when it resumes afterwards (fails with
# "CMake was unable to find a build program corresponding to Ninja").
# Installing a recent CMake upfront and forcing vcpkg to use system tools
# avoids the download-and-swap entirely.
RUN curl -fsSL https://github.com/Kitware/CMake/releases/download/v4.4.0/cmake-4.4.0-linux-x86_64.tar.gz \
      -o /tmp/cmake.tar.gz \
    && tar -xzf /tmp/cmake.tar.gz -C /usr/local --strip-components=1 \
    && rm /tmp/cmake.tar.gz
ENV VCPKG_FORCE_SYSTEM_BINARIES=1

WORKDIR /src
COPY CMakeLists.txt CMakePresets.json vcpkg.json vcpkg-configuration.json ./
COPY protos protos
COPY common common
COPY render_worker render_worker

# Cache vcpkg's built-package archive across builds/retries -- otherwise an
# interrupted build (or any change below this layer) re-downloads and
# recompiles all ~40 dependencies from scratch every time.
RUN --mount=type=cache,target=/root/.cache/vcpkg,id=vcpkg-cache \
    cmake --preset render-worker && cmake --build build-render-worker

FROM ubuntu:24.04 AS runtime

# libvulkan1 is the loader; mesa-vulkan-drivers provides the lavapipe (lvp)
# software ICD used for local testing. ca-certificates is for the AWS SDK's
# HTTPS calls (S3).
RUN apt-get update && apt-get install -y --no-install-recommends \
        libvulkan1 mesa-vulkan-drivers ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build-render-worker/render_worker/pathological ./pathological
COPY --from=builder /src/build-render-worker/render_worker/shaders ./shaders

# Pin the loader to lavapipe so it's picked even if other ICDs get added to
# the image later.
ENV VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json

EXPOSE 50051
ENTRYPOINT ["./pathological"]
