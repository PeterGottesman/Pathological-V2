#pragma once

#include "vulkan_context.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

struct Vertex {
    glm::vec3 position;
    float pad;
};

struct Material {
    glm::vec3 albedo;
    float pad0;
    glm::vec3 emission;
    float pad1;
};

class Scene {
public:
    explicit Scene(const VulkanContext& ctx);

    const Buffer& vertexBuffer() const { return *m_vertexBuffer; }
    const Buffer& indexBuffer() const { return *m_indexBuffer; }
    const Buffer& materialBuffer() const { return *m_materialBuffer; }
    const Buffer& materialIndexBuffer() const { return *m_materialIndexBuffer; }

    uint32_t indexCount() const { return m_indexCount; }
    uint32_t triangleCount() const { return m_indexCount / 3; }

private:
    void buildCornellBox();
    void uploadToGPU(const VulkanContext& ctx);

    // CPU-side data
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<Material> m_materials;
    std::vector<uint32_t> m_materialIndices; // Per-triangle material index

    // GPU buffers
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_indexBuffer;
    std::unique_ptr<Buffer> m_materialBuffer;
    std::unique_ptr<Buffer> m_materialIndexBuffer;

    uint32_t m_indexCount = 0;
};
