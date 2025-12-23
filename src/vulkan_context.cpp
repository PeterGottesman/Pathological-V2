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
    auto devices = vk::raii::PhysicalDevices(*m_instance);

    if (devices.empty()) {
        throw std::runtime_error("No Vulkan-capable GPU found");
    }

    const std::vector<const char*> requiredExtensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };

    for (const auto& device : devices) {
        auto properties = device.getProperties();

        // Prefer NVIDIA discrete GPU
        if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            continue;
        }

        // Check for required extensions
        auto availableExtensions = device.enumerateDeviceExtensionProperties();
        bool hasAllExtensions = true;

        for (const char* required : requiredExtensions) {
            bool found = false;
            for (const auto& available : availableExtensions) {
                if (strcmp(required, available.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                hasAllExtensions = false;
                break;
            }
        }

        if (!hasAllExtensions) {
            continue;
        }

        // Check for compute queue
        auto queueFamilies = device.getQueueFamilyProperties();
        bool hasComputeQueue = false;

        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
                m_queueFamilyIndex = i;
                hasComputeQueue = true;
                break;
            }
        }

        if (!hasComputeQueue) {
            continue;
        }

        // Found a suitable device
        m_physicalDevice = vk::raii::PhysicalDevice(device);

        // Get ray tracing properties using StructureChain
        auto props2Chain = m_physicalDevice->getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        m_rtProperties = props2Chain.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        std::cout << "Selected GPU: " << properties.deviceName << std::endl;
        return;
    }

    throw std::runtime_error("No suitable GPU with ray tracing support found");
}

void VulkanContext::createLogicalDevice() {
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        {},
        m_queueFamilyIndex,
        1,
        &queuePriority
    };

    // Required extensions
    std::vector<const char*> extensions = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    };

    // Enable required features via pNext chain
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelStructFeatures{};
    accelStructFeatures.accelerationStructure = VK_TRUE;
    accelStructFeatures.pNext = &bufferDeviceAddressFeatures;

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
    rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
    rtPipelineFeatures.pNext = &accelStructFeatures;

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.runtimeDescriptorArray = VK_TRUE;
    indexingFeatures.pNext = &rtPipelineFeatures;

    vk::PhysicalDeviceFeatures2 features2{};
    features2.pNext = &indexingFeatures;

    vk::DeviceCreateInfo deviceCreateInfo{
        {},
        queueCreateInfo,
        {},
        extensions,
        nullptr
    };
    deviceCreateInfo.pNext = &features2;

    m_device = vk::raii::Device(*m_physicalDevice, deviceCreateInfo);
    m_queue = vk::raii::Queue(*m_device, m_queueFamilyIndex, 0);
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
