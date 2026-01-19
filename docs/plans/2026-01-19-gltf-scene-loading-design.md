# glTF Scene Loading Design

## Overview

Add glTF scene loading to Pathological for student projects. Students implement a single integration function that wires together provided helpers, learning scene graph concepts with minimal C++ complexity.

## Goals

- Load glTF 2.0 files with triangle meshes and node hierarchies
- Support transform animations (translation, rotation, scale)
- Replace hardcoded Cornell box with dynamic scene loading
- Provide scaffolded learning experience for students with limited C++ experience

## Supported glTF Features

**Included:**
- Triangle meshes (positions, indices)
- Node hierarchy with transforms (translation, rotation, scale)
- Multiple meshes and instances
- Transform animations (first animation only)
- Basic materials (base color, emissive)

**Excluded:**
- Textures and texture coordinates
- Skeletal/skin animations
- Morph targets
- Multiple animation selection
- Cameras and lights
- Non-triangulated geometry

## Architecture

### Component Layers

**SceneGraph (new)** - High-level scene abstraction
- Factory: `SceneGraph::fromGltf(ctx, filename)` (student implements)
- Owns scene hierarchy and animation data
- `updateAnimation(time)` samples animation and updates transforms (provided)
- `build()` flattens hierarchy into Scene (provided)

**SceneBuilder (new)** - Mid-level builder with helpers
- Stores scene graph structure (nodes, transforms, parent/child relationships)
- Stores mesh and material data
- `loadGltfNode()` recursively builds hierarchy from glTF (provided)
- `createMesh/createMaterial` store data (provided)
- `flattenToScene()` traverses hierarchy, computes transforms, uploads to GPU (provided)

**Scene (modified)** - GPU resource container
- Remove hardcoded Cornell box construction
- Add constructor taking geometry/material vectors
- Owns GPU buffers
- Provides same interface to PathTracer

**PathTracer (unchanged)** - Still takes const Scene&

### Data Flow

```
glTF file → SceneGraph::fromGltf() → SceneBuilder → Scene → PathTracer
                    ↓
            updateAnimation(time)
```

## Student Implementation

Students implement only `SceneGraph::fromGltf()`:

```cpp
SceneGraph SceneGraph::fromGltf(const VulkanContext& ctx,
                                 const std::string& filename) {
    // 1. Load glTF file (provided wrapper)
    tinygltf::Model model = loadGltfFile(filename);

    SceneBuilder builder;

    // 2. Get default scene
    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    // 3. Iterate root nodes
    for (int nodeIdx : scene.nodes) {
        builder.loadGltfNode(model.nodes[nodeIdx], std::nullopt, model);
    }

    // 4. Store first animation if present
    std::optional<tinygltf::Animation> anim;
    if (!model.animations.empty()) {
        anim = model.animations[0];
    }

    return SceneGraph(builder, anim);
}
```

## Provided Implementation

### SceneGraph Class

```cpp
class SceneGraph {
public:
    static SceneGraph fromGltf(const VulkanContext& ctx,
                              const std::string& filename);

    void updateAnimation(float time);
    Scene build(const VulkanContext& ctx);

private:
    SceneGraph(SceneBuilder builder,
               std::optional<tinygltf::Animation> animation);

    SceneBuilder m_builder;
    std::optional<tinygltf::Animation> m_animation;
};
```

### SceneBuilder Class

```cpp
using NodeId = uint32_t;
using MeshId = uint32_t;
using MaterialId = uint32_t;

class SceneBuilder {
public:
    // Graph construction
    NodeId createNode(const std::string& name);
    void setParent(NodeId child, NodeId parent);
    void setLocalTransform(NodeId node, glm::mat4 transform);
    void setMesh(NodeId node, MeshId mesh);

    // Data creation
    MeshId createMesh(const std::vector<Vertex>& vertices,
                      const std::vector<uint32_t>& indices,
                      MaterialId material);
    MaterialId createMaterial(const Material& mat);

    // Helper (provided implementation)
    void loadGltfNode(const tinygltf::Node& gltfNode,
                      std::optional<NodeId> parent,
                      const tinygltf::Model& model);

    // Flattening (provided implementation)
    Scene flattenToScene(const VulkanContext& ctx);

private:
    struct Node {
        std::string name;
        glm::mat4 localTransform;
        std::optional<NodeId> parent;
        std::vector<NodeId> children;
        std::optional<MeshId> mesh;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        MaterialId materialId;
    };

    std::vector<Node> m_nodes;
    std::vector<MeshData> m_meshes;
    std::vector<Material> m_materials;
};
```

### Animation System

Animation sampling updates node transforms before flattening:

```cpp
void SceneGraph::updateAnimation(float time) {
    if (!m_animation.has_value()) return;

    const auto& anim = m_animation.value();

    for (const auto& channel : anim.channels) {
        NodeId nodeId = channel.target_node;

        if (channel.target_path == "translation") {
            glm::vec3 value = sampleChannel(anim, channel, time);
            m_builder.setNodeTranslation(nodeId, value);
        }
        else if (channel.target_path == "rotation") {
            glm::quat value = sampleQuatChannel(anim, channel, time);
            m_builder.setNodeRotation(nodeId, value);
        }
        else if (channel.target_path == "scale") {
            glm::vec3 value = sampleChannel(anim, channel, time);
            m_builder.setNodeScale(nodeId, value);
        }
    }
}
```

Helper functions:
- `sampleChannel()` - linearly interpolates between keyframes
- `sampleQuatChannel()` - spherically interpolates (slerp) for rotations
- `setNodeTranslation/Rotation/Scale()` - updates node's local transform matrix

### Transform Flattening

The flattening algorithm traverses nodes depth-first, accumulating transforms:

```cpp
Scene SceneBuilder::flattenToScene(const VulkanContext& ctx) {
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;
    std::vector<Material> allMaterials = m_materials;
    std::vector<uint32_t> allMaterialIndices;

    for (NodeId rootId : getRootNodes()) {
        traverseAndFlatten(rootId, glm::mat4(1.0f),
                          allVertices, allIndices,
                          allMaterials, allMaterialIndices);
    }

    return Scene(ctx, allVertices, allIndices,
                 allMaterials, allMaterialIndices);
}

void traverseAndFlatten(NodeId nodeId, glm::mat4 parentTransform, ...) {
    Node& node = m_nodes[nodeId];
    glm::mat4 worldTransform = parentTransform * node.localTransform;

    if (node.mesh.has_value()) {
        MeshData& mesh = m_meshes[node.mesh.value()];
        uint32_t baseVertex = allVertices.size();

        // Transform and append vertices
        for (const Vertex& v : mesh.vertices) {
            Vertex transformed = v;
            transformed.position = worldTransform * glm::vec4(v.position, 1.0f);
            allVertices.push_back(transformed);
        }

        // Append indices with base offset
        for (uint32_t idx : mesh.indices) {
            allIndices.push_back(baseVertex + idx);
        }

        // Material index per triangle
        for (size_t i = 0; i < mesh.indices.size() / 3; i++) {
            allMaterialIndices.push_back(mesh.materialId);
        }
    }

    for (NodeId child : node.children) {
        traverseAndFlatten(child, worldTransform, ...);
    }
}
```

### Material Conversion

```cpp
Material convertGltfMaterial(const tinygltf::Material& gltfMat) {
    Material mat;
    mat.albedo = glm::vec3(
        gltfMat.pbrMetallicRoughness.baseColorFactor[0],
        gltfMat.pbrMetallicRoughness.baseColorFactor[1],
        gltfMat.pbrMetallicRoughness.baseColorFactor[2]
    );
    mat.emission = glm::vec3(
        gltfMat.emissiveFactor[0],
        gltfMat.emissiveFactor[1],
        gltfMat.emissiveFactor[2]
    );
    return mat;
}
```

## Dependencies

Add to vcpkg.json:
```json
{
  "dependencies": [
    "tinygltf"
  ]
}
```

tinygltf is header-only and automatically includes:
- nlohmann/json (JSON parsing)
- stb_image (embedded texture support, unused but required)

## CLI Changes

Remove Cornell box, require glTF file, add time parameter:

```bash
./pathological <model.gltf> [options]

Options:
  -W, --width   Image width (default: 1024)
  -H, --height  Image height (default: 1024)
  -s, --samples Samples per pixel (default: 256)
  -o, --output  Output filename (default: output.png)
  -t, --time    Animation time in seconds (default: 0.0)

Example:
./pathological spinning_cube.gltf -t 1.5 -W 1920 -H 1080 -s 512 -o frame_045.png
```

## Error Handling

Provided `loadGltfFile()` wrapper handles:
- File not found
- Parse errors
- Missing scenes
- Warnings (printed to stderr)

Students should handle gracefully:
- Empty node lists (skip)
- Materials without emission (default to black)
- Meshes without indices (throw error - unsupported)

## Testing Strategy

Provide sample glTF files:
1. `static_cube.gltf` - Single cube, no animation
2. `spinning_cube.gltf` - Cube with rotation animation
3. `hierarchy.gltf` - Parent/child transforms
4. `multi_mesh.gltf` - Multiple meshes with different materials

Students verify:
- Static scenes render correctly
- Animation progresses with varying `-t` values
- Transforms accumulate correctly in hierarchies

## Learning Outcomes

Students learn:
- glTF structure (scenes, nodes, meshes, animations)
- Scene graph concepts (hierarchy, local vs world transforms)
- API integration (reading documentation, calling functions correctly)
- Separation of concerns (scene graph vs GPU resources)

## Future Extensions

Optional advanced topics:
- Multiple BLAS support for true instancing (rebuild TLAS, not BLAS)
- Texture support (requires UV coordinates, texture samplers in shaders)
- Normal mapping
- Camera loading from glTF (replace hardcoded camera)
- Multiple animation selection via CLI
