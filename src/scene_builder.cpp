#include "scene_builder.hpp"
#include <tiny_gltf.h>
#include <stdexcept>
#include <iostream>

NodeId SceneBuilder::createNode(const std::string& name) {
    NodeId id = m_nodes.size();
    Node node;
    node.name = name;
    node.localTransform = glm::mat4(1.0f);
    m_nodes.push_back(node);
    return id;
}

void SceneBuilder::setParent(NodeId child, NodeId parent) {
    if (child >= m_nodes.size() || parent >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setParent");
    }
    m_nodes[child].parent = parent;
    m_nodes[parent].children.push_back(child);
}

void SceneBuilder::setLocalTransform(NodeId node, const glm::mat4& transform) {
    if (node >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setLocalTransform");
    }
    m_nodes[node].localTransform = transform;
}

void SceneBuilder::setMesh(NodeId node, MeshId mesh) {
    if (node >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setMesh");
    }
    m_nodes[node].mesh = mesh;
}

MeshId SceneBuilder::createMesh(const std::vector<Vertex>& vertices,
                                const std::vector<uint32_t>& indices,
                                MaterialId material) {
    MeshId id = m_meshes.size();
    MeshData meshData;
    meshData.vertices = vertices;
    meshData.indices = indices;
    meshData.materialId = material;
    m_meshes.push_back(meshData);
    return id;
}

MaterialId SceneBuilder::createMaterial(const Material& mat) {
    MaterialId id = m_materials.size();
    m_materials.push_back(mat);
    return id;
}

void SceneBuilder::setNodeTranslation(NodeId node, const glm::vec3& translation) {
    if (node >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setNodeTranslation");
    }
    m_nodes[node].translation = translation;
    updateNodeTransform(node);
}

void SceneBuilder::setNodeRotation(NodeId node, const glm::quat& rotation) {
    if (node >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setNodeRotation");
    }
    m_nodes[node].rotation = rotation;
    updateNodeTransform(node);
}

void SceneBuilder::setNodeScale(NodeId node, const glm::vec3& scale) {
    if (node >= m_nodes.size()) {
        throw std::runtime_error("Invalid node ID in setNodeScale");
    }
    m_nodes[node].scale = scale;
    updateNodeTransform(node);
}

void SceneBuilder::updateNodeTransform(NodeId node) {
    Node& n = m_nodes[node];
    glm::mat4 T = glm::translate(glm::mat4(1.0f), n.translation);
    glm::mat4 R = glm::mat4_cast(n.rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), n.scale);
    n.localTransform = T * R * S;
}

std::vector<NodeId> SceneBuilder::getRootNodes() const {
    std::vector<NodeId> roots;
    for (NodeId i = 0; i < m_nodes.size(); ++i) {
        if (!m_nodes[i].parent.has_value()) {
            roots.push_back(i);
        }
    }
    return roots;
}

// Placeholder implementations - will be completed in next tasks
void SceneBuilder::loadGltfNode(const tinygltf::Node& gltfNode,
                                std::optional<NodeId> parent,
                                const tinygltf::Model& model) {
    // TODO: Implement in Task 4
    throw std::runtime_error("loadGltfNode not yet implemented");
}

Scene SceneBuilder::flattenToScene(const VulkanContext& ctx) {
    // TODO: Implement in Task 5
    throw std::runtime_error("flattenToScene not yet implemented");
}

void SceneBuilder::traverseAndFlatten(NodeId nodeId,
                                     const glm::mat4& parentTransform,
                                     std::vector<Vertex>& outVertices,
                                     std::vector<uint32_t>& outIndices,
                                     std::vector<uint32_t>& outMaterialIndices) {
    // TODO: Implement in Task 5
}
