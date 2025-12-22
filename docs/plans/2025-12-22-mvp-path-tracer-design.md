# MVP Path Tracer Design

## Overview

Headless Vulkan 1.3 path tracer rendering a Cornell box scene to PNG. Uses NVIDIA ray tracing extensions for hardware-accelerated rendering.

## Requirements

- Vulkan 1.3 compatible NVIDIA GPU with ray tracing support
- Hard-coded Cornell box scene
- Emissive and Lambertian materials only
- No textures
- Headless (no window) - output to PNG file
- vcpkg for package management

## Dependencies

**vcpkg.json:**
```json
{
  "dependencies": [
    "vulkan-memory-allocator",
    "glm",
    "cli11",
    "stb"
  ]
}
```

**External:**
- Vulkan SDK (installed system-wide)
- glslc shader compiler (from Vulkan SDK)

## Project Structure

```
pathological-v2/
├── CMakeLists.txt
├── vcpkg.json
├── src/
│   ├── main.cpp              # Entry point, CLI parsing, orchestration
│   ├── vulkan_context.hpp    # Vulkan setup declarations
│   ├── vulkan_context.cpp    # Instance, device, queues, VMA
│   ├── scene.hpp             # Scene data declarations
│   ├── scene.cpp             # Cornell box geometry & materials
│   ├── path_tracer.hpp       # Path tracer declarations
│   ├── path_tracer.cpp       # RT pipeline, acceleration structures, rendering
│   ├── image_output.hpp      # Image output declarations
│   └── image_output.cpp      # Buffer readback & PNG writing
└── shaders/
    ├── raygen.rgen           # Camera rays, sample accumulation
    ├── miss.rmiss            # Background color (black)
    └── closesthit.rchit      # Material evaluation, ray bouncing
```

## Command-Line Interface

```
pathological [options]
  -w, --width   <int>    Image width (default: 1024)
  -h, --height  <int>    Image height (default: 1024)
  -s, --samples <int>    Samples per pixel (default: 256)
  -o, --output  <path>   Output filename (default: output.png)
```

## Component Design

### VulkanContext

Handles headless Vulkan initialization:

**Instance:**
- Vulkan 1.3 API version
- No surface/presentation extensions
- Validation layers in debug builds

**Physical device selection:**
- NVIDIA GPU with ray tracing support
- Required extensions: `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_deferred_host_operations`

**Logical device:**
- Single queue family supporting compute
- Ray tracing pipeline features enabled
- Acceleration structure features enabled
- Buffer device address enabled

**VMA allocator:**
- Initialized with Vulkan 1.3 functions
- Used for all buffer/image allocations

**Exposes:**
- `vk::raii::Instance`
- `vk::raii::Device`
- `vk::raii::Queue`
- `VmaAllocator`
- Physical device properties (SBT alignment, etc.)

### Scene

Hard-coded Cornell box data:

**Geometry:**
- 5 quads: floor, ceiling, back wall, left wall (red), right wall (green)
- 2 boxes: tall box, short box (each as 5 quads)
- 1 light quad on ceiling (emissive)

**GPU buffers:**
- Vertex buffer: `vec3` positions
- Index buffer: triangle indices (quads split into 2 triangles)
- Material buffer: per-primitive material ID

**Material struct:**
```cpp
struct Material {
    glm::vec3 albedo;      // Diffuse color for Lambertian
    glm::vec3 emission;    // Non-zero for light sources
};
```

**Materials:**
- White Lambertian (floor, ceiling, back wall, boxes)
- Red Lambertian (left wall)
- Green Lambertian (right wall)
- Emissive white (ceiling light)

### PathTracer

**Acceleration structures:**

BLAS:
- Single BLAS containing all Cornell box triangles
- Built from vertex/index buffers
- Opaque geometry flag

TLAS:
- Single instance referencing BLAS with identity transform

**Build process:**
1. Create vertex/index buffers (device local)
2. Get BLAS size requirements, allocate scratch + result buffers
3. Build BLAS
4. Create TLAS instance buffer, build TLAS

**Ray tracing pipeline:**

Shader stages:
- Ray generation (.rgen)
- Miss (.rmiss)
- Closest hit (.rchit)

Pipeline:
- 1 ray generation group
- 1 miss group
- 1 hit group (closest hit only)
- Max recursion depth = 1 (iterative bouncing in raygen)

**Shader Binding Table:**
- Regions: raygen | miss | hit
- Single entry per region
- Proper alignment from physical device properties

**Descriptor set:**
- Binding 0: TLAS (acceleration structure)
- Binding 1: Output image (storage image, RGBA8)
- Binding 2: Material buffer (storage buffer)

**Push constants:**
- Camera origin and derived vectors
- Image dimensions
- Samples per pixel
- RNG seed

### Shaders

**raygen.rgen:**
```
For each pixel:
  accumulated_color = vec3(0)
  for sample in 0..SPP:
    jitter pixel position for anti-aliasing
    generate camera ray
    throughput = vec3(1)
    for bounce in 0..MAX_BOUNCES:
      traceRay(...)
      if payload.done: break
      accumulated_color += throughput * payload.emission
      throughput *= payload.color
    accumulated_color += throughput * payload.color
  output_image[pixel] = gamma_correct(accumulated_color / SPP)
```

**miss.rmiss:**
```
payload.color = vec3(0)
payload.done = true
```

**closesthit.rchit:**
```
Fetch material using gl_PrimitiveID
If emissive:
  payload.color = emission
  payload.done = true
If Lambertian:
  Compute hit point and normal
  Generate cosine-weighted hemisphere direction
  payload.color = albedo
  payload.origin = hit_point + epsilon * normal
  payload.direction = random_direction
  payload.done = false
```

**Ray payload:**
```glsl
struct RayPayload {
    vec3 color;
    vec3 origin;
    vec3 direction;
    bool done;
    uint seed;
};
```

### ImageOutput

**Readback:**
1. Create staging buffer (host visible + coherent)
2. Transition output image to TRANSFER_SRC_OPTIMAL
3. Copy image to staging buffer
4. Map buffer, write PNG via stb_image_write

**Gamma correction:**
Applied in shader (pow 1/2.2) before writing to RGBA8 output.

## CMake Build

```cmake
cmake_minimum_required(VERSION 3.20)
project(pathological)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
    src/scene.cpp
    src/path_tracer.cpp
    src/image_output.cpp
)

target_link_libraries(pathological PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    CLI11::CLI11
)

target_include_directories(pathological PRIVATE ${Stb_INCLUDE_DIR})

# Shader compilation
file(GLOB SHADERS shaders/*.rgen shaders/*.rmiss shaders/*.rchit)
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv
        COMMAND glslc ${SHADER} -o ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv
        DEPENDS ${SHADER}
    )
    list(APPEND SPV_SHADERS ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
endforeach()
add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_dependencies(pathological shaders)
```

## Main Flow

```cpp
int main(int argc, char** argv) {
    // 1. Parse CLI arguments
    auto config = parse_args(argc, argv);

    // 2. Initialize Vulkan context
    VulkanContext ctx;

    // 3. Build scene
    Scene scene(ctx);

    // 4. Create path tracer
    PathTracer tracer(ctx, scene, config.width, config.height);

    // 5. Render
    tracer.render(config.samples);

    // 6. Write output
    tracer.save_image(config.output);

    return 0;
}
```

## Key Design Decisions

1. **vulkan.hpp RAII** - Automatic resource cleanup, less error-prone
2. **VMA** - Handles memory allocation complexity
3. **Single BLAS** - Simpler than multiple for static scene
4. **Iterative bouncing** - Recursion depth 1, loop in raygen shader
5. **Fixed 8 bounces** - Sufficient for Cornell box
6. **Build-time shaders** - glslc via CMake custom commands
7. **Single dispatch** - All SPP in one shader invocation
