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
    // Will implement in next task
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
