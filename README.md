# Pathological

Pathological is a distributed GPU path tracing renderer. A **scheduler** accepts
render job submissions and delegates them over gRPC to one or more **render
workers**, which do the actual path tracing on the GPU (Vulkan 1.3 hardware ray
tracing) and upload results to S3. A **frontend** web UI drives the whole thing.

## Features

- Hardware-accelerated ray tracing via Vulkan 1.3 RT extensions
- Distributed rendering: a scheduler dispatches jobs to a pool of render workers over gRPC
- glTF scene loading with animation support
- Headless rendering (no window required)
- Adaptive recursive tiling to prevent GPU timeouts on embedded platforms (e.g. Jetson Orin Nano)
- Scenes loaded from S3, rendered output (PNG) uploaded back to S3
- Web frontend for submitting jobs and viewing results

## Architecture

- **`scheduler/`** — accepts render job submissions over HTTP, tracks job/worker
  state, and dispatches work to render workers over gRPC.
- **`render_worker/`** — long-running process that receives jobs from the
  scheduler, loads the scene from S3, renders it with Vulkan hardware ray
  tracing, and uploads the output PNG to S3.
- **`frontend/`** — Next.js web UI for submitting jobs and viewing/downloading
  results; talks to the scheduler through its own API routes. See
  [`frontend/README.md`](frontend/README.md) for frontend-specific details.
- **`common/`** — code shared by the scheduler and render worker (currently the
  S3 upload/download manager).
- **`protos/`** — shared gRPC/protobuf definitions for the scheduler↔worker RPCs.

## Requirements

- CMake 3.20+
- Ninja (`ninja-build`)
- [vcpkg](https://vcpkg.io), 
- Vulkan SDK 1.3+ and a ray-tracing-capable GPU (`render_worker` only)
- AWS credentials for the `default` profile (both `scheduler` and
  `render_worker` link the AWS SDK for S3 access)
- Node.js + npm (`frontend` only) — see `frontend/package.json` for exact
  package versions (Next.js 16.1.6, React 19.2.3)

## Building

The C++ side has two components, `scheduler/` and `render_worker/`, which share the gRPC proto libraries in `protos/` and the `S3Manager` in `common/`. Everything is built from this directory — there's a single root `CMakeLists.txt` and `vcpkg.json`; `scheduler/` and `render_worker/` no longer have their own.

Build both together:
```bash
cmake --preset all
cmake --build build
```
This produces `build/scheduler/pathological-sched` and `build/render_worker/pathological`.

Or build just one — this only installs that component's vcpkg dependencies (via [vcpkg manifest features](https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode#features)), so e.g. building `render_worker` alone on a Jetson doesn't require Drogon or pull in scheduler-only packages:
```bash
cmake --preset scheduler        # or: cmake --preset render-worker
cmake --build build-scheduler   # or: cmake --build build-render-worker
```
`cmake --list-presets` shows all presets, including `-debug` variants of each.

All presets need `VCPKG_ROOT` set, CMake 3.20+, and ninja-build. `render_worker` additionally needs the Vulkan SDK and a ray-tracing-capable GPU. Both link AWS SDK for S3 access, using the `default` credentials profile.

`vcpkg.json` pins `tinygltf` to 3.0.0#1 via an override: GitHub's tarball for
the 2.9.7 tag our pinned baseline resolves to by default no longer matches
the SHA512 vcpkg recorded for it, so a fresh `vcpkg install` of that version
fails with a hash mismatch. 3.0.0 keeps the same tiny_gltf.h v2 API this
codebase uses. Safe to drop once the vcpkg baseline is bumped past whatever
upstream commit fixes this.

## Running the full stack

Run in this order after building.

### 1) Start the scheduler

```bash
./build/scheduler/pathological-sched --http-port 8080 --grpc-port 50052
```

Flags:
- `-a, --address` (HTTP/gRPC listen address, default `0.0.0.0`)
- `-p, --grpc-port` (default `50052`)
- `--http-port` (default `8080`)

### 2) Start a render worker

```bash
./build/render_worker/pathological 127.0.0.1:50052 --port 50051 --render-address 127.0.0.1 --name worker-1
```

Flags: 
- the scheduler address (`host:port`) is a required positional argument
- `-p, --port` is the port the worker's own gRPC server listens on (default `50051`)
- `-a, --render-address` is the address the scheduler should reach this worker at (default `127.0.0.1`)
- `-n, --name` is the worker's display name (default `test_client_1`).

To scale out, start multiple workers with different `--port`/`--name` values.

### 3) Start the frontend

```bash
cd frontend
npm install
npm run dev
```

Then open `http://localhost:3000`. See [`frontend/README.md`](frontend/README.md)
for frontend-only setup, project conventions, and troubleshooting.

## Deployment

Docker images and Kubernetes manifests for running the full stack (locally
via kind + software Vulkan + MinIO, or on a real GPU cluster) live in
[`deploy/`](deploy/README.md).

## Adaptive Tiling

The render worker uses **adaptive recursive subdivision** (`render_worker/src/path_tracer.cpp`)
to prevent GPU timeouts on embedded platforms:

- The image starts as one tile; tiles larger than a maximum size are split into 4 quadrants
- Subdivision continues recursively until every tile fits within the size limit
- Minimum tile size is 64×64 pixels (prevents excessive subdivision)
- Statistics on the range of tile sizes used are printed when verbose output is enabled

This currently isn't exposed as a runtime option: `render_worker/src/render_server.cpp`
calls the tracer with a fixed max tile size of 512 and verbose output off for
every job it receives from the scheduler. 
