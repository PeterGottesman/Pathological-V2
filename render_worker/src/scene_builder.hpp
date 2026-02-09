#pragma once

#include "scene.hpp"
#include "vulkan_context.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <optional>

namespace tinygltf {
    class Model;
    struct Node;
}

using NodeId = uint32_t;
using MeshId = uint32_t;
using MaterialId = uint32_t;

class SceneBuilder {
public:
    // Graph construction
    NodeId createNode(const std::string& name);
    void setParent(NodeId child, NodeId parent);
    void setLocalTransform(NodeId node, const glm::mat4& transform);
    void setMesh(NodeId node, MeshId mesh);

    // Data creation
    MeshId createMesh(const std::vector<Vertex>& vertices,
                      const std::vector<uint32_t>& indices,
                      MaterialId material);
    MaterialId createMaterial(const Material& mat);

    // Transform updates (for animation)
    void setNodeTranslation(NodeId node, const glm::vec3& translation);
    void setNodeRotation(NodeId node, const glm::quat& rotation);
    void setNodeScale(NodeId node, const glm::vec3& scale);

    // Helper to load glTF node hierarchy
    void loadGltfNode(const tinygltf::Node& gltfNode,
                      std::optional<NodeId> parent,
                      const tinygltf::Model& model);

    // Flattening
    Scene flattenToScene(const VulkanContext& ctx);

private:
    struct Node {
        std::string name;
        glm::mat4 localTransform;
        std::optional<NodeId> parent;
        std::vector<NodeId> children;
        std::optional<MeshId> mesh;

        // For animation (TRS components)
        glm::vec3 translation{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        MaterialId materialId;
    };

    std::vector<Node> m_nodes;
    std::vector<MeshData> m_meshes;
    std::vector<Material> m_materials;

    // Helper methods
    std::vector<NodeId> getRootNodes() const;
    void traverseAndFlatten(NodeId nodeId,
                           const glm::mat4& parentTransform,
                           std::vector<Vertex>& outVertices,
                           std::vector<uint32_t>& outIndices,
                           std::vector<uint32_t>& outMaterialIndices);
    void updateNodeTransform(NodeId node);
};
