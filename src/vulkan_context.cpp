#include "vulkan_context.hpp"

#include <iostream>
#include <stdexcept>
#include <cstring>

// Required for VMA implementation
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

} // namespace

VulkanContext::VulkanContext() {
    createInstance();
    selectPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createAllocator();
}

VulkanContext::~VulkanContext() {
    if (m_allocator) {
        vmaDestroyAllocator(m_allocator);
    }
}

void VulkanContext::createInstance() {
    vk::ApplicationInfo appInfo{
        "Pathological",
        VK_MAKE_VERSION(1, 0, 0),
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3
    };

    std::vector<const char*> layers;
    std::vector<const char*> extensions;

#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    vk::InstanceCreateInfo createInfo{
        {},
        &appInfo,
        layers,
        extensions
    };

    m_instance = vk::raii::Instance(m_context, createInfo);
}

void VulkanContext::selectPhysicalDevice() {
    // Implemented in next step
}

void VulkanContext::createLogicalDevice() {
    // Implemented in next step
}

void VulkanContext::createCommandPool() {
    // Implemented in next step
}

void VulkanContext::createAllocator() {
    // Implemented in next step
}

void VulkanContext::executeCommands(std::function<void(vk::raii::CommandBuffer&)> func) const {
    // Implemented later
}
