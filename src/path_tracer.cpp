#include "path_tracer.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>

PathTracer::PathTracer(const VulkanContext& ctx, const Scene& scene,
                       uint32_t width, uint32_t height)
    : m_ctx(ctx), m_scene(scene), m_width(width), m_height(height)
{
    createOutputImage();
    createAccelerationStructures();
    createRayTracingPipeline();
    createShaderBindingTable();
    createDescriptorSets();
}

PathTracer::~PathTracer() {
    if (m_outputImage) {
        vmaDestroyImage(m_ctx.allocator(), m_outputImage, m_outputImageAllocation);
    }
}

void PathTracer::createOutputImage() {
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {m_width, m_height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image;
    if (vmaCreateImage(m_ctx.allocator(), &imageInfo, &allocInfo,
                       &image, &m_outputImageAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create output image");
    }
    m_outputImage = image;

    // Create image view
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = m_outputImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eR8G8B8A8Unorm;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    m_outputImageView = vk::raii::ImageView(m_ctx.device(), viewInfo);

    // Transition to general layout
    m_ctx.executeCommands([this](vk::raii::CommandBuffer& cmd) {
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eGeneral;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_outputImage;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            {},
            nullptr, nullptr, barrier
        );
    });
}

void PathTracer::createAccelerationStructures() {
    // === Build BLAS ===

    vk::DeviceAddress vertexAddress = m_scene.vertexBuffer().deviceAddress(*m_ctx.device());
    vk::DeviceAddress indexAddress = m_scene.indexBuffer().deviceAddress(*m_ctx.device());

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{};
    trianglesData.vertexFormat = vk::Format::eR32G32B32Sfloat;
    trianglesData.vertexData.deviceAddress = vertexAddress;
    trianglesData.vertexStride = sizeof(Vertex);
    trianglesData.maxVertex = static_cast<uint32_t>(m_scene.indexCount());
    trianglesData.indexType = vk::IndexType::eUint32;
    trianglesData.indexData.deviceAddress = indexAddress;

    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
    geometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
    geometry.geometry.triangles = trianglesData;

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
    buildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    uint32_t primitiveCount = m_scene.triangleCount();

    auto sizeInfo = m_ctx.device().getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfo,
        primitiveCount
    );

    // Create BLAS buffer
    m_blasBuffer = std::make_unique<Buffer>(
        m_ctx.allocator(),
        sizeInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Create BLAS
    vk::AccelerationStructureCreateInfoKHR blasCreateInfo{};
    blasCreateInfo.buffer = m_blasBuffer->buffer();
    blasCreateInfo.size = sizeInfo.accelerationStructureSize;
    blasCreateInfo.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

    m_blas = vk::raii::AccelerationStructureKHR(m_ctx.device(), blasCreateInfo);

    // Create scratch buffer
    Buffer scratchBuffer(
        m_ctx.allocator(),
        sizeInfo.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    buildInfo.dstAccelerationStructure = **m_blas;
    buildInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress(*m_ctx.device());

    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = primitiveCount;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex = 0;
    rangeInfo.transformOffset = 0;

    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    m_ctx.executeCommands([&](vk::raii::CommandBuffer& cmd) {
        cmd.buildAccelerationStructuresKHR(buildInfo, pRangeInfo);
    });

    // === Build TLAS ===

    vk::DeviceAddress blasAddress = m_ctx.device().getAccelerationStructureAddressKHR({**m_blas});

    vk::AccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = 1.0f;
    instance.transform.matrix[1][1] = 1.0f;
    instance.transform.matrix[2][2] = 1.0f;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = static_cast<uint32_t>(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
    instance.accelerationStructureReference = blasAddress;

    m_instanceBuffer = std::make_unique<Buffer>(
        m_ctx.allocator(),
        sizeof(vk::AccelerationStructureInstanceKHR),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    m_instanceBuffer->upload(&instance, 1);

    vk::AccelerationStructureGeometryInstancesDataKHR instancesData{};
    instancesData.arrayOfPointers = VK_FALSE;
    instancesData.data.deviceAddress = m_instanceBuffer->deviceAddress(*m_ctx.device());

    vk::AccelerationStructureGeometryKHR tlasGeometry{};
    tlasGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
    tlasGeometry.geometry.instances = instancesData;

    vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{};
    tlasBuildInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    tlasBuildInfo.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    tlasBuildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    tlasBuildInfo.geometryCount = 1;
    tlasBuildInfo.pGeometries = &tlasGeometry;

    auto tlasSizeInfo = m_ctx.device().getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        tlasBuildInfo,
        1u
    );

    m_tlasBuffer = std::make_unique<Buffer>(
        m_ctx.allocator(),
        tlasSizeInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{};
    tlasCreateInfo.buffer = m_tlasBuffer->buffer();
    tlasCreateInfo.size = tlasSizeInfo.accelerationStructureSize;
    tlasCreateInfo.type = vk::AccelerationStructureTypeKHR::eTopLevel;

    m_tlas = vk::raii::AccelerationStructureKHR(m_ctx.device(), tlasCreateInfo);

    Buffer tlasScratchBuffer(
        m_ctx.allocator(),
        tlasSizeInfo.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    tlasBuildInfo.dstAccelerationStructure = **m_tlas;
    tlasBuildInfo.scratchData.deviceAddress = tlasScratchBuffer.deviceAddress(*m_ctx.device());

    vk::AccelerationStructureBuildRangeInfoKHR tlasRangeInfo{};
    tlasRangeInfo.primitiveCount = 1;
    const vk::AccelerationStructureBuildRangeInfoKHR* pTlasRangeInfo = &tlasRangeInfo;

    m_ctx.executeCommands([&](vk::raii::CommandBuffer& cmd) {
        cmd.buildAccelerationStructuresKHR(tlasBuildInfo, pTlasRangeInfo);
    });

    std::cout << "Acceleration structures built successfully" << std::endl;
}

void PathTracer::createRayTracingPipeline() {
    // Will implement later
}

void PathTracer::createShaderBindingTable() {
    // Will implement later
}

void PathTracer::createDescriptorSets() {
    // Will implement later
}

void PathTracer::render(uint32_t samplesPerPixel) {
    // Will implement later
}

void PathTracer::saveImage(const std::string& filename) {
    // Will implement later
}

std::vector<char> PathTracer::loadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

vk::raii::ShaderModule PathTracer::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return vk::raii::ShaderModule(m_ctx.device(), createInfo);
}
