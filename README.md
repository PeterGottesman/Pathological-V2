# Pathological

Pathological is a Vulkan 1.3 path tracer using hardware ray tracing.

## Features

- Hardware-accelerated ray tracing via Vulkan RT extensions
- Cornell box scene with emissive and Lambertian materials
- Headless rendering (no window required)
- PNG output

## Requirements

- Vulkan SDK 1.3+
- NVIDIA GPU with ray tracing support
- vcpkg
- CMake 3.20+
- ninja-build

## Building

```bash
cmake --preset default
cmake --build build
```

## Usage

```bash
./pathological [options]

Options:
  -W, --width   Image width (default: 1024)
  -H, --height  Image height (default: 1024)
  -s, --samples Samples per pixel (default: 256)
  -o, --output  Output filename (default: output.png)
```

## Example

```bash
cd build
./pathological -W 1920 -H 1080 -s 16 -o render.png
```

# Student Project Spring 2026
- Implement server/client architecture
  - Implement server (C++ or python)
  - Create cli client (C++ or python)
  - Create web client
- Graphics features:
  - Camera positioning
  - Texture loading & sampling
  - Cosine weighted hemisphere sampling
  - Animated lights
- Video rendering

- Use S3 interface for object storage
