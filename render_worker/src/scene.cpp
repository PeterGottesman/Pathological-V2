#include "scene.hpp"

Scene::Scene(const VulkanContext& ctx,
             const std::vector<Vertex>& vertices,
             const std::vector<uint32_t>& indices,
             const std::vector<Material>& materials,
             const std::vector<uint32_t>& materialIndices)
    : m_vertexCount(vertices.size()), m_indexCount(indices.size())
{
    // Create vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        sizeof(Vertex) * vertices.size(),
        vk::BufferUsageFlagBits::eVertexBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_vertexBuffer->upload(vertices.data(), vertices.size());

    // Create index buffer
    m_indexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        sizeof(uint32_t) * indices.size(),
        vk::BufferUsageFlagBits::eIndexBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eStorageBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_indexBuffer->upload(indices.data(), indices.size());

    // Create material buffer
    m_materialBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        sizeof(Material) * materials.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_materialBuffer->upload(materials.data(), materials.size());

    // Create material index buffer
    m_materialIndexBuffer = std::make_unique<Buffer>(
        ctx.allocator(),
        sizeof(uint32_t) * materialIndices.size(),
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_materialIndexBuffer->upload(materialIndices.data(), materialIndices.size());
}
