#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

#include <optional>
#include <vector>
#include <string>

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // Non-copyable, non-movable (VMA allocator complicates moves)
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    // Accessors
    const vk::raii::Instance& instance() const { return *m_instance; }
    const vk::raii::PhysicalDevice& physicalDevice() const { return *m_physicalDevice; }
    const vk::raii::Device& device() const { return *m_device; }
    const vk::raii::Queue& queue() const { return *m_queue; }
    const vk::raii::CommandPool& commandPool() const { return *m_commandPool; }
    uint32_t queueFamilyIndex() const { return m_queueFamilyIndex; }
    VmaAllocator allocator() const { return m_allocator; }

    // Ray tracing properties
    const vk::PhysicalDeviceRayTracingPipelinePropertiesKHR& rtProperties() const {
        return m_rtProperties;
    }

    // Utility: run a one-shot command
    void executeCommands(std::function<void(vk::raii::CommandBuffer&)> func) const;

private:
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createAllocator();

    vk::raii::Context m_context;
    std::optional<vk::raii::Instance> m_instance;
    std::optional<vk::raii::PhysicalDevice> m_physicalDevice;
    std::optional<vk::raii::Device> m_device;
    std::optional<vk::raii::Queue> m_queue;
    std::optional<vk::raii::CommandPool> m_commandPool;

    uint32_t m_queueFamilyIndex = 0;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{};
};
