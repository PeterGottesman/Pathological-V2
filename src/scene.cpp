#include "scene.hpp"

namespace {

// Add a quad (two triangles) to the scene
void addQuad(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
             std::vector<uint32_t>& materialIndices, uint32_t materialIndex,
             const glm::vec3& v0, const glm::vec3& v1,
             const glm::vec3& v2, const glm::vec3& v3)
{
    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    vertices.push_back({v0});
    vertices.push_back({v1});
    vertices.push_back({v2});
    vertices.push_back({v3});

    // Triangle 1
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    materialIndices.push_back(materialIndex);

    // Triangle 2
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
    materialIndices.push_back(materialIndex);
}

// Add a box (5 visible faces, no bottom)
void addBox(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
            std::vector<uint32_t>& materialIndices, uint32_t materialIndex,
            const glm::vec3& min, const glm::vec3& max)
{
    // Front face
    addQuad(vertices, indices, materialIndices, materialIndex,
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {max.x, max.y, max.z}, {min.x, max.y, max.z});

    // Back face
    addQuad(vertices, indices, materialIndices, materialIndex,
            {max.x, min.y, min.z}, {min.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z});

    // Left face
    addQuad(vertices, indices, materialIndices, materialIndex,
            {min.x, min.y, min.z}, {min.x, min.y, max.z},
            {min.x, max.y, max.z}, {min.x, max.y, min.z});

    // Right face
    addQuad(vertices, indices, materialIndices, materialIndex,
            {max.x, min.y, max.z}, {max.x, min.y, min.z},
            {max.x, max.y, min.z}, {max.x, max.y, max.z});

    // Top face
    addQuad(vertices, indices, materialIndices, materialIndex,
            {min.x, max.y, max.z}, {max.x, max.y, max.z},
            {max.x, max.y, min.z}, {min.x, max.y, min.z});
}

} // namespace

Scene::Scene(const VulkanContext& ctx) {
    buildCornellBox();
    uploadToGPU(ctx);
}

void Scene::buildCornellBox() {
    // Materials: 0=white, 1=red, 2=green, 3=light
    m_materials = {
        {{0.73f, 0.73f, 0.73f}, 0.0f, {0.0f, 0.0f, 0.0f}, 0.0f},  // White
        {{0.65f, 0.05f, 0.05f}, 0.0f, {0.0f, 0.0f, 0.0f}, 0.0f},  // Red
        {{0.12f, 0.45f, 0.15f}, 0.0f, {0.0f, 0.0f, 0.0f}, 0.0f},  // Green
        {{0.0f, 0.0f, 0.0f}, 0.0f, {15.0f, 15.0f, 15.0f}, 0.0f},  // Light
    };

    // Cornell box dimensions (scaled to reasonable size)
    const float size = 2.0f;
    const float half = size / 2.0f;

    // Floor (white)
    addQuad(m_vertices, m_indices, m_materialIndices, 0,
            {-half, 0.0f, -half}, {half, 0.0f, -half},
            {half, 0.0f, half}, {-half, 0.0f, half});

    // Ceiling (white)
    addQuad(m_vertices, m_indices, m_materialIndices, 0,
            {-half, size, half}, {half, size, half},
            {half, size, -half}, {-half, size, -half});

    // Back wall (white)
    addQuad(m_vertices, m_indices, m_materialIndices, 0,
            {-half, 0.0f, -half}, {-half, size, -half},
            {half, size, -half}, {half, 0.0f, -half});

    // Left wall (red)
    addQuad(m_vertices, m_indices, m_materialIndices, 1,
            {-half, 0.0f, half}, {-half, size, half},
            {-half, size, -half}, {-half, 0.0f, -half});

    // Right wall (green)
    addQuad(m_vertices, m_indices, m_materialIndices, 2,
            {half, 0.0f, -half}, {half, size, -half},
            {half, size, half}, {half, 0.0f, half});

    // Tall box (white)
    addBox(m_vertices, m_indices, m_materialIndices, 0,
           {-0.6f, 0.0f, -0.6f}, {-0.1f, 1.2f, -0.1f});

    // Short box (white)
    addBox(m_vertices, m_indices, m_materialIndices, 0,
           {0.1f, 0.0f, 0.1f}, {0.6f, 0.6f, 0.6f});

    // Light (on ceiling)
    addQuad(m_vertices, m_indices, m_materialIndices, 3,
            {-0.3f, size - 0.001f, -0.3f}, {0.3f, size - 0.001f, -0.3f},
            {0.3f, size - 0.001f, 0.3f}, {-0.3f, size - 0.001f, 0.3f});

    m_indexCount = static_cast<uint32_t>(m_indices.size());
}

void Scene::uploadToGPU(const VulkanContext& ctx) {
    auto usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                 vk::BufferUsageFlagBits::eShaderDeviceAddress |
                 vk::BufferUsageFlagBits::eStorageBuffer;

    // Vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        m_vertices.size() * sizeof(Vertex),
        usage,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_vertexBuffer->upload(m_vertices.data(), m_vertices.size());

    // Index buffer
    m_indexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        m_indices.size() * sizeof(uint32_t),
        usage,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_indexBuffer->upload(m_indices.data(), m_indices.size());

    // Material buffer
    m_materialBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        m_materials.size() * sizeof(Material),
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_materialBuffer->upload(m_materials.data(), m_materials.size());

    // Material index buffer (per-triangle)
    m_materialIndexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        m_materialIndices.size() * sizeof(uint32_t),
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_materialIndexBuffer->upload(m_materialIndices.data(), m_materialIndices.size());
}
