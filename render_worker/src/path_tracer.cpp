#include "path_tracer.hpp"
#include "vulkan/vulkan.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

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
    trianglesData.maxVertex = static_cast<uint32_t>(m_scene.vertexCount() - 1);
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
    // Load shaders
    auto raygenCode = loadShader("shaders/raygen.rgen.spv");
    auto missCode = loadShader("shaders/miss.rmiss.spv");
    auto chitCode = loadShader("shaders/closesthit.rchit.spv");

    auto raygenModule = createShaderModule(raygenCode);
    auto missModule = createShaderModule(missCode);
    auto chitModule = createShaderModule(chitCode);

    // Shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> stages;

    vk::PipelineShaderStageCreateInfo raygenStage{};
    raygenStage.stage = vk::ShaderStageFlagBits::eRaygenKHR;
    raygenStage.module = *raygenModule;
    raygenStage.pName = "main";
    stages.push_back(raygenStage);

    vk::PipelineShaderStageCreateInfo missStage{};
    missStage.stage = vk::ShaderStageFlagBits::eMissKHR;
    missStage.module = *missModule;
    missStage.pName = "main";
    stages.push_back(missStage);

    vk::PipelineShaderStageCreateInfo chitStage{};
    chitStage.stage = vk::ShaderStageFlagBits::eClosestHitKHR;
    chitStage.module = *chitModule;
    chitStage.pName = "main";
    stages.push_back(chitStage);

    // Shader groups
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups;

    // Raygen group
    vk::RayTracingShaderGroupCreateInfoKHR raygenGroup{};
    raygenGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    raygenGroup.generalShader = 0;
    raygenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    groups.push_back(raygenGroup);

    // Miss group
    vk::RayTracingShaderGroupCreateInfoKHR missGroup{};
    missGroup.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    missGroup.generalShader = 1;
    missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    groups.push_back(missGroup);

    // Hit group
    vk::RayTracingShaderGroupCreateInfoKHR hitGroup{};
    hitGroup.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
    hitGroup.closestHitShader = 2;
    hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    groups.push_back(hitGroup);

    // Descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eRaygenKHR},
        {1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eRaygenKHR},
        {2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eClosestHitKHR},
        {3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eClosestHitKHR},
        {4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eClosestHitKHR},
        {5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eClosestHitKHR},
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_descriptorSetLayout = vk::raii::DescriptorSetLayout(m_ctx.device(), layoutInfo);

    // Pipeline layout
    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    vk::DescriptorSetLayout setLayout = **m_descriptorSetLayout;
    pipelineLayoutInfo.pSetLayouts = &setLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    m_pipelineLayout = vk::raii::PipelineLayout(m_ctx.device(), pipelineLayoutInfo);

    // Create ray tracing pipeline
    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout = **m_pipelineLayout;

    m_pipeline = vk::raii::Pipeline(
        m_ctx.device(),
        nullptr,  // deferred operation
        nullptr,  // pipeline cache
        pipelineInfo
    );

    std::cout << "Ray tracing pipeline created" << std::endl;
}

void PathTracer::createShaderBindingTable() {
    const auto& rtProps = m_ctx.rtProperties();
    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;

    uint32_t handleSizeAligned = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);
    uint32_t handleSizeBaseAligned = (handleSize + baseAlignment - 1) & ~(baseAlignment - 1);

    uint32_t groupCount = 3; // raygen, miss, hit
    uint32_t sbtSize = groupCount * handleSizeBaseAligned;

    // Get shader group handles
    auto handleData = m_pipeline->getRayTracingShaderGroupHandlesKHR<uint8_t>(
        0, groupCount, groupCount * handleSize
    );
    std::vector<uint8_t> handles = handleData;

    // Create SBT buffer
    m_sbtBuffer = std::make_unique<Buffer>(
        m_ctx.allocator(),
        sbtSize,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    // Copy handles to SBT buffer with proper alignment
    uint8_t* sbtData = static_cast<uint8_t*>(m_sbtBuffer->map());
    for (uint32_t i = 0; i < groupCount; i++) {
        std::memcpy(sbtData + i * handleSizeBaseAligned,
                    handles.data() + i * handleSize,
                    handleSize);
    }
    m_sbtBuffer->unmap();

    vk::DeviceAddress sbtAddress = m_sbtBuffer->deviceAddress(*m_ctx.device());

    m_raygenRegion.deviceAddress = sbtAddress;
    m_raygenRegion.stride = handleSizeAligned;
    m_raygenRegion.size = handleSizeAligned;

    m_missRegion.deviceAddress = sbtAddress + handleSizeBaseAligned;
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size = handleSizeAligned;

    m_hitRegion.deviceAddress = sbtAddress + 2 * handleSizeBaseAligned;
    m_hitRegion.stride = handleSizeAligned;
    m_hitRegion.size = handleSizeAligned;

    m_callableRegion = vk::StridedDeviceAddressRegionKHR{};

    std::cout << "Shader binding table created" << std::endl;
}

void PathTracer::createDescriptorSets() {
    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eAccelerationStructureKHR, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 4},
    };

	vk::DescriptorPoolCreateFlags poolCreateFlags{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet};
    vk::DescriptorPoolCreateInfo poolInfo{poolCreateFlags};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    m_descriptorPool = vk::raii::DescriptorPool(m_ctx.device(), poolInfo);

    // Allocate descriptor set
    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = **m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    vk::DescriptorSetLayout setLayout = **m_descriptorSetLayout;
    allocInfo.pSetLayouts = &setLayout;

    auto sets = vk::raii::DescriptorSets(m_ctx.device(), allocInfo);
    m_descriptorSet = std::move(sets[0]);

    // Update descriptor set
    vk::WriteDescriptorSetAccelerationStructureKHR asInfo{};
    asInfo.accelerationStructureCount = 1;
    vk::AccelerationStructureKHR tlas = **m_tlas;
    asInfo.pAccelerationStructures = &tlas;

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageView = **m_outputImageView;
    imageInfo.imageLayout = vk::ImageLayout::eGeneral;

    vk::DescriptorBufferInfo vertexInfo{m_scene.vertexBuffer().buffer(), 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo indexInfo{m_scene.indexBuffer().buffer(), 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo materialInfo{m_scene.materialBuffer().buffer(), 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo matIndexInfo{m_scene.materialIndexBuffer().buffer(), 0, VK_WHOLE_SIZE};

    std::vector<vk::WriteDescriptorSet> writes;

    vk::WriteDescriptorSet asWrite{};
    asWrite.dstSet = **m_descriptorSet;
    asWrite.dstBinding = 0;
    asWrite.descriptorCount = 1;
    asWrite.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
    asWrite.pNext = &asInfo;
    writes.push_back(asWrite);

    vk::WriteDescriptorSet imageWrite{};
    imageWrite.dstSet = **m_descriptorSet;
    imageWrite.dstBinding = 1;
    imageWrite.descriptorCount = 1;
    imageWrite.descriptorType = vk::DescriptorType::eStorageImage;
    imageWrite.pImageInfo = &imageInfo;
    writes.push_back(imageWrite);

    vk::WriteDescriptorSet vertexWrite{};
    vertexWrite.dstSet = **m_descriptorSet;
    vertexWrite.dstBinding = 2;
    vertexWrite.descriptorCount = 1;
    vertexWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    vertexWrite.pBufferInfo = &vertexInfo;
    writes.push_back(vertexWrite);

    vk::WriteDescriptorSet indexWrite{};
    indexWrite.dstSet = **m_descriptorSet;
    indexWrite.dstBinding = 3;
    indexWrite.descriptorCount = 1;
    indexWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    indexWrite.pBufferInfo = &indexInfo;
    writes.push_back(indexWrite);

    vk::WriteDescriptorSet materialWrite{};
    materialWrite.dstSet = **m_descriptorSet;
    materialWrite.dstBinding = 4;
    materialWrite.descriptorCount = 1;
    materialWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    materialWrite.pBufferInfo = &materialInfo;
    writes.push_back(materialWrite);

    vk::WriteDescriptorSet matIndexWrite{};
    matIndexWrite.dstSet = **m_descriptorSet;
    matIndexWrite.dstBinding = 5;
    matIndexWrite.descriptorCount = 1;
    matIndexWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    matIndexWrite.pBufferInfo = &matIndexInfo;
    writes.push_back(matIndexWrite);

    m_ctx.device().updateDescriptorSets(writes, nullptr);

    std::cout << "Descriptor sets created" << std::endl;
}

void PathTracer::render(uint32_t samplesPerPixel, uint32_t tileSize, bool verbose) {
    // tileSize and verbose will be used in tiled rendering implementation
    (void)tileSize;
    (void)verbose;

    std::cout << "Rendering " << m_width << "x" << m_height
              << " with " << samplesPerPixel << " samples per pixel..." << std::endl;

    // Render will be implemented in task 4
    // For now, just call renderTileRegion for the full image
    renderTileRegion(0, 0, m_width, m_height, samplesPerPixel);

    std::cout << "Rendering complete" << std::endl;
}

void PathTracer::renderTileRegion(uint32_t offsetX, uint32_t offsetY,
                                   uint32_t width, uint32_t height,
                                   uint32_t samplesPerPixel) {
    // Set up camera (looking at scene center)
    PushConstants pc{};
    pc.cameraPosition = glm::vec3(0.0f, 0.0f, 3.5f);
    pc.fov = glm::radians(45.0f);

    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    pc.cameraForward = glm::normalize(target - pc.cameraPosition);
    pc.cameraRight = glm::normalize(glm::cross(pc.cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));
    pc.cameraUp = glm::cross(pc.cameraRight, pc.cameraForward);

    pc.samplesPerPixel = samplesPerPixel;
    pc.frameIndex = 0;
    pc.maxBounces = 8;
    pc.tileOffset = glm::uvec2(offsetX, offsetY);
    pc.imageSize = glm::uvec2(m_width, m_height);

    m_ctx.executeCommands([&](vk::raii::CommandBuffer& cmd) {
        cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, **m_pipeline);

        vk::DescriptorSet descSet = **m_descriptorSet;
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            **m_pipelineLayout,
            0,
            descSet,
            nullptr
        );

        cmd.pushConstants<PushConstants>(
            **m_pipelineLayout,
            vk::ShaderStageFlagBits::eRaygenKHR,
            0,
            pc
        );

        cmd.traceRaysKHR(
            m_raygenRegion,
            m_missRegion,
            m_hitRegion,
            m_callableRegion,
            width,
            height,
            1
        );
    });
}

void PathTracer::saveImage(const std::string& filename) {
    std::cout << "Saving image to " << filename << "..." << std::endl;

    // Create staging buffer
    vk::DeviceSize imageSize = m_width * m_height * 4;
    Buffer stagingBuffer(
        m_ctx.allocator(),
        imageSize,
        vk::BufferUsageFlagBits::eTransferDst,
        VMA_MEMORY_USAGE_GPU_TO_CPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
    );

    // Copy image to staging buffer
    m_ctx.executeCommands([&](vk::raii::CommandBuffer& cmd) {
        // Transition image to transfer src
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_outputImage;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            nullptr, nullptr, barrier
        );

        // Copy image to buffer
        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset.x = 0;
        region.imageOffset.y = 0;
        region.imageOffset.z = 0;
        region.imageExtent.width = m_width;
        region.imageExtent.height = m_height;
        region.imageExtent.depth = 1;

        cmd.copyImageToBuffer(
            m_outputImage,
            vk::ImageLayout::eTransferSrcOptimal,
            stagingBuffer.buffer(),
            region
        );
    });

    // Read back and save
    uint8_t* data = static_cast<uint8_t*>(stagingBuffer.map());
    stbi_write_png(filename.c_str(), m_width, m_height, 4, data, m_width * 4);
    stagingBuffer.unmap();

    std::cout << "Image saved" << std::endl;
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
