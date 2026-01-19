# Pathological: Vulkan Path Tracer with glTF Scene Loading

## Project Overview

Pathological is a headless Vulkan 1.3 path tracer that renders scenes loaded from glTF 2.0 files. It uses NVIDIA ray tracing extensions for high-performance ray tracing with Monte Carlo sampling.

**Key Features:**
- Vulkan 1.3 with ray tracing (VK_KHR_ray_tracing_pipeline)
- glTF 2.0 scene loading with node hierarchies and animations
- Transform animation support (translation, rotation, scale)
- Material properties: albedo and emission
- Monte Carlo path tracing with Russian roulette termination
- Outputs to PNG using stb_image_write

**Tech Stack:**
- C++20 with vulkan.hpp RAII wrappers
- Vulkan Memory Allocator (VMA) for GPU memory management
- GLM for mathematics
- CLI11 for command-line argument parsing
- tinygltf for glTF 2.0 file loading
- glslc for shader compilation to SPIR-V
- stb libraries for image I/O

## glTF Scene Loading Pipeline

The glTF loading pipeline follows a three-layer architecture:

```
glTF File
    ↓
SceneGraph::fromGltf() → Parses glTF, builds node hierarchy
    ↓
SceneBuilder → Intermediate structure with scene graph and mesh data
    ↓
Scene → GPU resource container with buffers and acceleration structures
    ↓
PathTracer → Renders using ray tracing pipeline
```

### Layer 1: SceneGraph

**Purpose:** Represent the dynamic scene with animation support.

**API:**
```cpp
class SceneGraph {
    static SceneGraph fromGltf(const VulkanContext& ctx, const std::string& filename);
    void updateAnimation(float time);
    Scene build(const VulkanContext& ctx);
};
```

**Responsibilities:**
- Load glTF file using tinygltf
- Extract default scene and its node hierarchy
- Store first animation (if present)
- Provide updateAnimation() to sample keyframes at any time
- Provide build() to flatten hierarchy and create GPU resources

### Layer 2: SceneBuilder

**Purpose:** Intermediate builder that stores scene hierarchy, meshes, and materials before GPU upload.

**API:**
```cpp
class SceneBuilder {
    // Node management
    NodeId createNode(const std::string& name);
    void setParent(NodeId child, NodeId parent);
    void setLocalTransform(NodeId node, glm::mat4 transform);
    void setMesh(NodeId node, MeshId mesh);

    // Data creation
    MeshId createMesh(const std::vector<Vertex>& vertices,
                      const std::vector<uint32_t>& indices,
                      MaterialId material);
    MaterialId createMaterial(const Material& mat);

    // Helper (provided)
    void loadGltfNode(const tinygltf::Node& gltfNode,
                      std::optional<NodeId> parent,
                      const tinygltf::Model& model);

    // Flattening (provided)
    Scene flattenToScene(const VulkanContext& ctx);
};
```

**Responsibilities:**
- Store scene graph as hierarchical nodes
- Recursively load glTF nodes via loadGltfNode()
- Extract vertex positions and indices from glTF accessors/bufferViews
- Convert glTF materials to Material structs
- Support both explicit matrix and TRS (translation/rotation/scale) transforms
- Traverse hierarchy and flatten to world space for GPU upload

### Layer 3: Scene

**Purpose:** GPU-side resource container.

**API:**
```cpp
class Scene {
    Scene(const VulkanContext& ctx,
          const std::vector<Vertex>& vertices,
          const std::vector<uint32_t>& indices,
          const std::vector<Material>& materials,
          const std::vector<uint32_t>& materialIndices);

    // Accessors
    const Buffer& vertexBuffer() const;
    const Buffer& indexBuffer() const;
    const Buffer& materialBuffer() const;
    const Buffer& materialIndexBuffer() const;
    uint32_t indexCount() const;
};
```

**Responsibilities:**
- Create GPU buffers for vertices, indices, and materials
- Each triangle knows which material it uses via materialIndices
- Provide device addresses for ray tracing

## Supported glTF Features

### Included
- Triangle meshes with vertex positions
- Node hierarchies with local/world transform tracking
- Multiple meshes per scene
- Transform animations (first animation only)
- Basic materials: base color (albedo) and emissive factor
- Both .gltf (JSON) and .glb (binary) formats

### Excluded (Future Extensions)
- Texture coordinates and textures
- Skeletal/skin animations
- Morph targets
- Multiple animation selection via CLI
- Cameras and lights (hardcoded camera in path tracer)
- Non-triangulated geometry

## CLI Usage

```bash
./pathological <model.gltf> [options]

Positional Arguments:
  model.gltf                    Path to glTF 2.0 scene file (required)

Options:
  -W, --width <uint>            Image width (default: 1024)
  -H, --height <uint>           Image height (default: 1024)
  -s, --samples <uint>          Samples per pixel (default: 256)
  -t, --time <float>            Animation time in seconds (default: 0.0)
  -o, --output <string>         Output PNG filename (default: output.png)
```

### Examples

Render a static scene:
```bash
./pathological models/cube.gltf -W 1920 -H 1080 -s 512 -o render.png
```

Render animated scene at specific time:
```bash
./pathological models/spinner.gltf -t 2.5 -W 1920 -H 1080 -s 1024 -o frame_045.png
```

Test with minimal settings:
```bash
./pathological models/simple.gltf -W 256 -H 256 -s 4 -o test.png
```

## Architecture Components

### VulkanContext

Manages Vulkan device initialization and GPU resources.

**Responsibilities:**
- Create Vulkan instance with validation layers (debug builds)
- Select GPU (prefers discrete NVIDIA with ray tracing support, falls back to others)
- Create logical device with ray tracing extensions
- Manage VMA allocator for GPU memory
- Provide utility methods for synchronous command execution

**Key Extensions:**
- VK_KHR_acceleration_structure
- VK_KHR_ray_tracing_pipeline
- VK_KHR_deferred_host_operations
- VK_KHR_buffer_device_address
- VK_EXT_descriptor_indexing
- VK_KHR_spirv_1_4
- VK_KHR_shader_float_controls

### PathTracer

Manages ray tracing pipeline and rendering.

**Responsibilities:**
- Create output image and image views
- Build bottom-level acceleration structures (BLAS) for geometry
- Build top-level acceleration structure (TLAS) for scene
- Create ray tracing pipeline with raygen/miss/closesthit shaders
- Create shader binding table for dispatch
- Render via ray tracing dispatch commands
- Copy rendered image to staging buffer and save to PNG

**Render Loop:**
1. Issue render command: `traceRaysKHR()` dispatch
2. For each sample: accumulate color with Russian roulette path termination
3. Gamma-correct accumulated result
4. Write to output image

### Shaders

**raygen.rgen (Ray Generation)**
- Entry point for ray tracing
- Generates primary rays from camera
- Implements Monte Carlo path tracing with importance sampling
- Accumulates color over samples

**miss.rmiss (Miss Shader)**
- Returns zero radiance (no environment lighting)
- Terminates paths that miss all geometry

**closesthit.rchit (Closest Hit)**
- Computes hit properties (position, normal via barycentric interpolation)
- Fetches material from material buffer
- Checks for emissive surfaces (light sources)
- For diffuse: samples new direction in hemisphere
- Returns color and new ray origin/direction for recursion

## Data Structures

### Material
```cpp
struct Material {
    glm::vec3 albedo;      // Diffuse base color
    glm::vec3 emission;    // Self-illumination
};
```

### Vertex
```cpp
struct Vertex {
    glm::vec3 position;
    float pad;             // Alignment padding for GPU buffer
};
```

### Transform Representation
Nodes can specify transforms as:
1. **Matrix**: 4x4 explicit transformation matrix
2. **TRS**: Translation (vec3) + Rotation (quat) + Scale (vec3)

SceneBuilder converts both to 4x4 matrices internally.

## Animation System

Animations are sampled at a specified time before rendering:

```cpp
sceneGraph.updateAnimation(time);  // Sample all channels at time=2.5s
Scene scene = sceneGraph.build(ctx);  // Flatten with animated transforms
```

**Supported Animation Channels:**
- `translation` (vec3) - Linear interpolation between keyframes
- `rotation` (quat) - Spherical linear interpolation (SLERP)
- `scale` (vec3) - Linear interpolation

**Limitations:**
- Only first animation in the glTF file is loaded
- Keyframe interpolation assumes LINEAR type (step/cubic not supported)
- Animation loops are not handled (user provides time in seconds)

## Transform Flattening Algorithm

When building the Scene, the SceneBuilder traverses the node hierarchy depth-first and accumulates transforms:

```
For each root node:
    traverseAndFlatten(nodeId, parentTransform):
        worldTransform = parentTransform × nodeLocalTransform

        if node has mesh:
            for each vertex in mesh:
                vertex.position = worldTransform × vertex.position
            append transformed vertices to allVertices
            append indices (with base offset) to allIndices
            append material index for each triangle

        for each child:
            traverseAndFlatten(childId, worldTransform)
```

This produces flat vertex/index/material arrays suitable for GPU upload.

## Building

### Prerequisites
- CMake 3.20+
- C++20 compiler (gcc/clang)
- Vulkan SDK 1.3+
- vcpkg for dependency management

### Configuration
```bash
cmake --preset default
```

This:
- Finds all vcpkg dependencies (tinygltf, glm, CLI11, etc.)
- Configures shader compilation to SPIR-V
- Sets up build directory

### Building
```bash
cmake --build build --config Release
```

Shader compilation happens automatically as part of the build.

### Output
- `build/pathological` - Main executable
- `build/shaders/` - Compiled SPIR-V shaders

## Testing

Test with the provided cube scene:
```bash
cd build
./pathological ../test_scenes/cube.gltf -W 512 -H 512 -s 64 -o cube_render.png
```

## Performance Notes

- **GPU Memory:** Each BLAS acceleration structure requires ~1-2x vertex buffer size
- **Rendering:** Highly parallelizable; scales with GPU core count
- **Path Depth:** Limited to ~8-10 bounces before numerical instability on single-precision floats
- **Sample Count:** 256-512 samples needed for noise-free images
- **Render Time:** ~seconds for 512x512@256spp on discrete NVIDIA GPU

## Error Handling

**File Loading:**
- Missing glTF file → Exception with filename in message
- Parse errors → tinygltf error details included
- Missing default scene → Exception

**Geometry Issues:**
- Empty scenes (no nodes/meshes) → Exception
- Non-triangle primitives → Warning, skipped
- Missing POSITION attribute → No vertices extracted, empty geometry
- Missing indices → Treated as non-indexed (error for now)

**GPU Errors:**
- Missing ray tracing support → Exception during VulkanContext init
- Out of GPU memory → VMA allocation failure exception
- Shader compilation errors → Printed to stderr

## Future Extensions

**Near-term:**
- Animation keyframe interpolation (step, linear, cubic)
- Multiple animation selection via CLI (`-a animation_name`)
- Camera loading from glTF

**Medium-term:**
- Texture support (UV coordinates, samplers, base color textures)
- Normal maps for surface detail
- Roughness/metallic from materials (PBR)
- Multiple BLAS with true GPU instancing

**Long-term:**
- Denoising filters for reduced sample count
- Progressive refinement / interactive preview
- glTF extensions: Draco compression, KTX textures
- Bidirectional path tracing
