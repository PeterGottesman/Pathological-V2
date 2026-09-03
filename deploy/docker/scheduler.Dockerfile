# syntax=docker/dockerfile:1
# Build context is the repo root, e.g.:
#   docker build -f deploy/docker/scheduler.Dockerfile -t pathological-scheduler .

FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl zip unzip tar pkg-config \
        python3 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# vcpkg itself, pinned to nothing in particular here -- vcpkg.json's own
# baseline (vcpkg-configuration.json) controls dependency versions.
RUN git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg

WORKDIR /src
COPY CMakeLists.txt CMakePresets.json vcpkg.json vcpkg-configuration.json ./
COPY deploy/vcpkg-overlay-triplets deploy/vcpkg-overlay-triplets
COPY protos protos
COPY common common
COPY scheduler scheduler

# vcpkg manifest install runs automatically as part of configure. Cache its
# built-package archive across builds/retries (shared with
# render_worker.Dockerfile's build, since most dependencies overlap) --
# otherwise an interrupted build re-downloads and recompiles everything.
# VCPKG_OVERLAY_TRIPLETS skips building the debug variant of every
# dependency -- a deployed container never needs it.
RUN --mount=type=cache,target=/root/.cache/vcpkg,id=vcpkg-cache-release \
    cmake --preset scheduler -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_OVERLAY_TRIPLETS=/src/deploy/vcpkg-overlay-triplets \
    && cmake --build build-scheduler

FROM ubuntu:24.04 AS runtime

# vcpkg's default x64-linux triplet links dependencies statically, so the
# runtime image only needs CA certs for the AWS SDK's HTTPS calls (S3).
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build-scheduler/scheduler/pathological-sched ./pathological-sched

EXPOSE 8080 50052
ENTRYPOINT ["./pathological-sched"]
