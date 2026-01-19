#include "scene_builder.hpp"
#include <tiny_gltf.h>
#include <stdexcept>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

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

static Material convertGltfMaterial(const tinygltf::Material& gltfMat) {
    Material mat;

    // Base color (albedo)
    const auto& baseColor = gltfMat.pbrMetallicRoughness.baseColorFactor;
    mat.albedo = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);

    // Emission
    const auto& emission = gltfMat.emissiveFactor;
    mat.emission = glm::vec3(emission[0], emission[1], emission[2]);

    return mat;
}

static glm::mat4 getGltfNodeTransform(const tinygltf::Node& gltfNode) {
    glm::mat4 transform(1.0f);

    if (gltfNode.matrix.size() == 16) {
        // Matrix provided directly
        transform = glm::make_mat4(gltfNode.matrix.data());
    } else {
        // Compose from TRS
        glm::vec3 translation(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale(1.0f);

        if (gltfNode.translation.size() == 3) {
            translation = glm::vec3(gltfNode.translation[0],
                                   gltfNode.translation[1],
                                   gltfNode.translation[2]);
        }

        if (gltfNode.rotation.size() == 4) {
            // glTF uses [x, y, z, w], glm uses [w, x, y, z]
            rotation = glm::quat(gltfNode.rotation[3],
                                gltfNode.rotation[0],
                                gltfNode.rotation[1],
                                gltfNode.rotation[2]);
        }

        if (gltfNode.scale.size() == 3) {
            scale = glm::vec3(gltfNode.scale[0],
                             gltfNode.scale[1],
                             gltfNode.scale[2]);
        }

        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        transform = T * R * S;
    }

    return transform;
}

void SceneBuilder::loadGltfNode(const tinygltf::Node& gltfNode,
                                std::optional<NodeId> parent,
                                const tinygltf::Model& model) {
    // Create node
    NodeId nodeId = createNode(gltfNode.name);

    // Set transform
    glm::mat4 transform = getGltfNodeTransform(gltfNode);
    setLocalTransform(nodeId, transform);

    // Set parent relationship
    if (parent.has_value()) {
        setParent(nodeId, parent.value());
    }

    // Load mesh if present
    if (gltfNode.mesh >= 0) {
        const tinygltf::Mesh& gltfMesh = model.meshes[gltfNode.mesh];

        // Process each primitive (we only handle first primitive for simplicity)
        if (!gltfMesh.primitives.empty()) {
            const tinygltf::Primitive& primitive = gltfMesh.primitives[0];

            // Only support triangles
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                std::cerr << "Warning: Skipping non-triangle primitive in mesh "
                         << gltfMesh.name << std::endl;
                return;
            }

            // Extract vertices (positions)
            std::vector<Vertex> vertices;
            auto posIt = primitive.attributes.find("POSITION");
            if (posIt != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[posIt->second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                const float* positions = reinterpret_cast<const float*>(
                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]);

                vertices.resize(accessor.count);
                for (size_t i = 0; i < accessor.count; ++i) {
                    vertices[i].position = glm::vec3(
                        positions[i * 3 + 0],
                        positions[i * 3 + 1],
                        positions[i * 3 + 2]
                    );
                    vertices[i].pad = 0.0f;
                }
            }

            // Extract indices
            std::vector<uint32_t> indices;
            if (primitive.indices >= 0) {
                const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                indices.resize(accessor.count);

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* idx = reinterpret_cast<const uint16_t*>(
                        &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                    for (size_t i = 0; i < accessor.count; ++i) {
                        indices[i] = idx[i];
                    }
                } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* idx = reinterpret_cast<const uint32_t*>(
                        &buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                    std::copy(idx, idx + accessor.count, indices.begin());
                }
            }

            // Create material
            MaterialId matId;
            if (primitive.material >= 0) {
                Material mat = convertGltfMaterial(model.materials[primitive.material]);
                matId = createMaterial(mat);
            } else {
                // Default material (white diffuse)
                Material mat;
                mat.albedo = glm::vec3(0.8f, 0.8f, 0.8f);
                mat.emission = glm::vec3(0.0f);
                matId = createMaterial(mat);
            }

            // Create mesh and attach to node
            MeshId meshId = createMesh(vertices, indices, matId);
            setMesh(nodeId, meshId);
        }
    }

    // Recursively load children
    for (int childIdx : gltfNode.children) {
        loadGltfNode(model.nodes[childIdx], nodeId, model);
    }
}

Scene SceneBuilder::flattenToScene(const VulkanContext& ctx) {
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;
    std::vector<uint32_t> allMaterialIndices;

    // Traverse from each root node
    std::vector<NodeId> roots = getRootNodes();
    for (NodeId rootId : roots) {
        traverseAndFlatten(rootId, glm::mat4(1.0f),
                          allVertices, allIndices, allMaterialIndices);
    }

    // Handle empty scene
    if (allVertices.empty()) {
        throw std::runtime_error("Scene has no geometry");
    }

    return Scene(ctx, allVertices, allIndices, m_materials, allMaterialIndices);
}

void SceneBuilder::traverseAndFlatten(NodeId nodeId,
                                     const glm::mat4& parentTransform,
                                     std::vector<Vertex>& outVertices,
                                     std::vector<uint32_t>& outIndices,
                                     std::vector<uint32_t>& outMaterialIndices) {
    const Node& node = m_nodes[nodeId];
    glm::mat4 worldTransform = parentTransform * node.localTransform;

    // If node has a mesh, transform and append vertices
    if (node.mesh.has_value()) {
        const MeshData& mesh = m_meshes[node.mesh.value()];
        uint32_t baseVertex = outVertices.size();

        // Transform vertices to world space
        for (const Vertex& v : mesh.vertices) {
            Vertex transformed;
            glm::vec4 worldPos = worldTransform * glm::vec4(v.position, 1.0f);
            transformed.position = glm::vec3(worldPos);
            transformed.pad = 0.0f;
            outVertices.push_back(transformed);
        }

        // Append indices with base offset
        for (uint32_t idx : mesh.indices) {
            outIndices.push_back(baseVertex + idx);
        }

        // Add material index for each triangle
        uint32_t triangleCount = mesh.indices.size() / 3;
        for (uint32_t i = 0; i < triangleCount; ++i) {
            outMaterialIndices.push_back(mesh.materialId);
        }
    }

    // Recursively traverse children
    for (NodeId child : node.children) {
        traverseAndFlatten(child, worldTransform, outVertices, outIndices, outMaterialIndices);
    }
}
