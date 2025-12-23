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

## Building

```bash
cmake --preset default
cmake --build build
```

## Usage

```bash
./build/pathological [options]

Options:
  -W, --width   Image width (default: 1024)
  -H, --height  Image height (default: 1024)
  -s, --samples Samples per pixel (default: 256)
  -o, --output  Output filename (default: output.png)
```

## Example

```bash
./build/pathological -W 1920 -H 1080 -s 512 -o render.png
```
