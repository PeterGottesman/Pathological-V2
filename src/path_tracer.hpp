#pragma once

#include "vulkan_context.hpp"
#include "scene.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

struct PushConstants {
    glm::vec3 cameraPosition;
    float fov;
    glm::vec3 cameraForward;
    uint32_t samplesPerPixel;
    glm::vec3 cameraRight;
    uint32_t frameIndex;
    glm::vec3 cameraUp;
    uint32_t maxBounces;
    glm::uvec2 tileOffset;      // Tile offset in pixels
    glm::uvec2 imageSize;       // Full image dimensions
};

class PathTracer {
public:
    PathTracer(const VulkanContext& ctx, const Scene& scene,
               uint32_t width, uint32_t height);
    ~PathTracer();

    void render(uint32_t samplesPerPixel, uint32_t tileSize, bool verbose);
    void saveImage(const std::string& filename);

private:
    struct SubdivisionStats {
        uint32_t minTileSize = UINT32_MAX;
        uint32_t maxTileSize = 0;
        uint32_t totalTiles = 0;
        uint32_t subdivisions = 0;
    };

    void createOutputImage();
    void createAccelerationStructures();
    void createRayTracingPipeline();
    void createShaderBindingTable();
    void createDescriptorSets();

    void renderTileRegion(uint32_t offsetX, uint32_t offsetY,
                          uint32_t width, uint32_t height,
                          uint32_t samplesPerPixel);

    void renderTileRecursive(uint32_t offsetX, uint32_t offsetY,
                             uint32_t width, uint32_t height,
                             uint32_t samplesPerPixel,
                             uint32_t maxTileSize,
                             bool verbose,
                             SubdivisionStats& stats);

    std::vector<char> loadShader(const std::string& filename);
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code);

    const VulkanContext& m_ctx;
    const Scene& m_scene;
    uint32_t m_width;
    uint32_t m_height;

    // Output image
    vk::Image m_outputImage;
    VmaAllocation m_outputImageAllocation = VK_NULL_HANDLE;
    std::optional<vk::raii::ImageView> m_outputImageView;

    // Acceleration structures
    std::unique_ptr<Buffer> m_blasBuffer;
    std::optional<vk::raii::AccelerationStructureKHR> m_blas;
    std::unique_ptr<Buffer> m_tlasBuffer;
    std::optional<vk::raii::AccelerationStructureKHR> m_tlas;
    std::unique_ptr<Buffer> m_instanceBuffer;

    // Pipeline
    std::optional<vk::raii::PipelineLayout> m_pipelineLayout;
    std::optional<vk::raii::Pipeline> m_pipeline;
    std::optional<vk::raii::DescriptorSetLayout> m_descriptorSetLayout;
    std::optional<vk::raii::DescriptorPool> m_descriptorPool;
    std::optional<vk::raii::DescriptorSet> m_descriptorSet;

    // Shader binding table
    std::unique_ptr<Buffer> m_sbtBuffer;
    vk::StridedDeviceAddressRegionKHR m_raygenRegion{};
    vk::StridedDeviceAddressRegionKHR m_missRegion{};
    vk::StridedDeviceAddressRegionKHR m_hitRegion{};
    vk::StridedDeviceAddressRegionKHR m_callableRegion{};
};
