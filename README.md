# Pathological

Pathological is a Vulkan 1.3 path tracer using hardware ray tracing.

## Features

- Hardware-accelerated ray tracing via Vulkan RT extensions
- glTF scene loading with animation support
- Headless rendering (no window required)
- Adaptive recursive tiling to prevent GPU timeouts on embedded platforms
- Automatic subdivision based on tile size threshold
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
./pathological <gltf-file> [options]

Options:
  -W, --width      Image width (default: 1024)
  -H, --height     Image height (default: 1024)
  -s, --samples    Samples per pixel (default: 256)
  -o, --output     Output filename (default: output.png)
  -t, --time       Animation time in seconds (default: 0.0)
  --tile-size      Maximum tile size before subdivision (default: 512)
  -v, --verbose    Enable verbose output
```

## Examples

Basic rendering:
```bash
cd build
./pathological test_scenes/cornell_box.gltf -W 1920 -H 1080 -s 16 -o render.png
```

Jetson Orin Nano (prevent GPU timeout):
```bash
# Smaller max tile size for platforms with strict GPU timeouts
./pathological test_scenes/cornell_box.gltf --tile-size 256 -v
```

Adaptive subdivision with verbose output:
```bash
# Show subdivision decisions and statistics
./pathological test_scenes/cornell_box.gltf -s 512 --tile-size 512 -v
```

## Adaptive Tiling

The path tracer uses **adaptive recursive subdivision** to prevent GPU timeouts:

- Images are subdivided into tiles based on the `--tile-size` threshold
- Tiles larger than the threshold are automatically split into 4 quadrants
- Subdivision continues recursively until tiles fit within the size limit
- Minimum tile size is 64×64 pixels (prevents excessive subdivision)
- Statistics show the range of tile sizes used during rendering

**How it works:**
1. Start with full image as one tile
2. If tile dimensions exceed `--tile-size`, subdivide into quadrants
3. Recursively process each quadrant
4. Render tiles that fit within the threshold
5. Display subdivision statistics at completion

This ensures that individual GPU submissions complete within timeout limits on embedded platforms like the Jetson Orin Nano, while automatically adapting to any image size and complexity.

# Student Project Spring 2026
- Implement server/client architecture
  - Implement scheduler (C++ or python)
  - Implement render worker (C++ or python)
  - Create cli client (C++ or python) or web client (some kinda javascript I'd assume)
- Graphics features (complete some):
  - Camera positioning
  - Texture loading & sampling
  - Cosine weighted hemisphere sampling
  - Animated lights
- Get scenes to render from S3
- Output rendered scenes to S3

## Architecture
There will be an scheduler server that gets connections
from client interfaces and render workers. It will:
- Track the state of ongoing renders
- Track the state of render workers
- Delegate work to render workers via gRPC
- Provide an interface for clients to submit a render job to

The render workers will be long running processes which can:
- Initiate & Manage gRPC communication with the scheduler
- Manage loading scene data from S3 object store
- Manage storing output images/movies to S3 object store
- Render the given scene on GPU using Vulkan
- NOTE: You can adapt the existing C++ application to have all of the
  above functionality, or you can add another application which
  creates a renderer subprocess. The latter approach is good if you
  want to do this in python or similar.

The client interface will allow users to:
- Submit render requests
- View the status of render requests
- View or download the outputs of renders
- I would prefer a CLI application, but a web app is fine as well
