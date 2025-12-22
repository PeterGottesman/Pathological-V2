# MVP Path Tracer Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a headless Vulkan 1.3 path tracer that renders a Cornell box to PNG using NVIDIA ray tracing extensions.

**Architecture:** VulkanContext handles device setup, Scene provides Cornell box geometry, PathTracer manages acceleration structures and ray tracing pipeline, ImageOutput writes results to PNG. All components use vulkan.hpp RAII wrappers and VMA for memory management.

**Tech Stack:** C++20, Vulkan 1.3, vulkan.hpp RAII, VMA, GLM, CLI11, stb_image_write, glslc

---

## Task 1: Update Build Configuration

**Files:**
- Modify: `vcpkg.json`
- Modify: `CMakeLists.txt`
- Create: `shaders/.gitkeep`

**Step 1: Update vcpkg.json with new dependencies**

Replace the contents of `vcpkg.json`:

```json
{
  "name": "pathological",
  "version": "0.1.0",
  "dependencies": [
    "vulkan-memory-allocator",
    "glm",
    "cli11",
    "stb"
  ]
}
```

**Step 2: Create shaders directory**

```bash
mkdir -p shaders
touch shaders/.gitkeep
```

**Step 3: Update CMakeLists.txt**

Replace the contents of `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)

project(pathological)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Main executable
add_executable(pathological
    src/main.cpp
)

target_link_libraries(pathological PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    glm::glm
    CLI11::CLI11
)

target_include_directories(pathological PRIVATE ${Stb_INCLUDE_DIR})

# Shader compilation
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/raygen.rgen
    ${SHADER_SOURCE_DIR}/miss.rmiss
    ${SHADER_SOURCE_DIR}/closesthit.rchit
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND Vulkan::glslc ${SHADER} -o ${SHADER_OUTPUT} --target-env=vulkan1.3
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPV_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_dependencies(pathological shaders)

# Copy shaders to output directory for runtime access
add_custom_command(TARGET pathological POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_BINARY_DIR} $<TARGET_FILE_DIR:pathological>/shaders
)
```

**Step 4: Reconfigure and verify build system**

```bash
rm -rf build
cmake --preset default
```

Expected: Configuration succeeds, finds all packages.

**Step 5: Commit**

```bash
git add vcpkg.json CMakeLists.txt shaders/.gitkeep
git commit -m "chore: update build config for path tracer dependencies"
```

---

## Task 2: Create VulkanContext Header

**Files:**
- Create: `src/vulkan_context.hpp`

**Step 1: Create the VulkanContext header**

Create `src/vulkan_context.hpp`:

```cpp
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
```

**Step 2: Commit**

```bash
git add src/vulkan_context.hpp
git commit -m "feat: add VulkanContext header"
```

---

## Task 3: Implement VulkanContext - Instance Creation

**Files:**
- Create: `src/vulkan_context.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Create vulkan_context.cpp with instance creation**

Create `src/vulkan_context.cpp`:

```cpp
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
```

**Step 2: Add vulkan_context.cpp to CMakeLists.txt**

In `CMakeLists.txt`, update the add_executable line:

```cmake
add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
)
```

**Step 3: Verify it compiles**

```bash
cmake --build build 2>&1 | head -50
```

Expected: Compiles (with linker errors for unimplemented functions, that's OK).

**Step 4: Commit**

```bash
git add src/vulkan_context.cpp CMakeLists.txt
git commit -m "feat: implement VulkanContext instance creation"
```

---

## Task 4: Implement VulkanContext - Physical Device Selection

**Files:**
- Modify: `src/vulkan_context.cpp`

**Step 1: Implement selectPhysicalDevice**

Replace the `selectPhysicalDevice` function in `src/vulkan_context.cpp`:

```cpp
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

        // Get ray tracing properties
        vk::PhysicalDeviceProperties2 props2;
        props2.pNext = &m_rtProperties;
        m_physicalDevice->getProperties2(&props2);

        std::cout << "Selected GPU: " << properties.deviceName << std::endl;
        return;
    }

    throw std::runtime_error("No suitable GPU with ray tracing support found");
}
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -50
```

Expected: Compiles successfully.

**Step 3: Commit**

```bash
git add src/vulkan_context.cpp
git commit -m "feat: implement VulkanContext physical device selection"
```

---

## Task 5: Implement VulkanContext - Logical Device Creation

**Files:**
- Modify: `src/vulkan_context.cpp`

**Step 1: Implement createLogicalDevice**

Replace the `createLogicalDevice` function in `src/vulkan_context.cpp`:

```cpp
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
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -50
```

Expected: Compiles successfully.

**Step 3: Commit**

```bash
git add src/vulkan_context.cpp
git commit -m "feat: implement VulkanContext logical device creation"
```

---

## Task 6: Implement VulkanContext - Command Pool and VMA

**Files:**
- Modify: `src/vulkan_context.cpp`

**Step 1: Implement createCommandPool and createAllocator**

Replace the `createCommandPool` and `createAllocator` functions in `src/vulkan_context.cpp`:

```cpp
void VulkanContext::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        m_queueFamilyIndex
    };

    m_commandPool = vk::raii::CommandPool(*m_device, poolInfo);
}

void VulkanContext::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.physicalDevice = **m_physicalDevice;
    allocatorInfo.device = **m_device;
    allocatorInfo.instance = **m_instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
}

void VulkanContext::executeCommands(std::function<void(vk::raii::CommandBuffer&)> func) const {
    vk::CommandBufferAllocateInfo allocInfo{
        **m_commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    };

    auto commandBuffers = vk::raii::CommandBuffers(*m_device, allocInfo);
    auto& cmd = commandBuffers[0];

    cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    func(cmd);
    cmd.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuf = *cmd;
    submitInfo.pCommandBuffers = &cmdBuf;

    m_queue->submit(submitInfo);
    m_queue->waitIdle();
}
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -50
```

Expected: Compiles successfully.

**Step 3: Commit**

```bash
git add src/vulkan_context.cpp
git commit -m "feat: implement VulkanContext command pool and VMA allocator"
```

---

## Task 7: Test VulkanContext Initialization

**Files:**
- Modify: `src/main.cpp`

**Step 1: Update main.cpp to test VulkanContext**

Replace the contents of `src/main.cpp`:

```cpp
#include "vulkan_context.hpp"

#include <iostream>

int main(int argc, char** argv) {
    try {
        std::cout << "Initializing Vulkan context..." << std::endl;
        VulkanContext ctx;
        std::cout << "Vulkan context initialized successfully!" << std::endl;

        std::cout << "Shader group handle size: "
                  << ctx.rtProperties().shaderGroupHandleSize << std::endl;
        std::cout << "Shader group base alignment: "
                  << ctx.rtProperties().shaderGroupBaseAlignment << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

**Step 2: Build and run**

```bash
cmake --build build && ./build/pathological
```

Expected: Prints GPU name and ray tracing properties.

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "test: verify VulkanContext initialization"
```

---

## Task 8: Create Buffer Utility Class

**Files:**
- Create: `src/buffer.hpp`

**Step 1: Create buffer.hpp**

Create `src/buffer.hpp`:

```cpp
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

#include <cstring>
#include <stdexcept>

class Buffer {
public:
    Buffer(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
           VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0)
        : m_allocator(allocator), m_size(size)
    {
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = static_cast<VkBufferUsageFlags>(usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = memoryUsage;
        allocInfo.flags = flags;

        VkBuffer buffer;
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &m_allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer");
        }
        m_buffer = buffer;
    }

    ~Buffer() {
        if (m_buffer) {
            vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        }
    }

    // Non-copyable, movable
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
        : m_allocator(other.m_allocator), m_buffer(other.m_buffer),
          m_allocation(other.m_allocation), m_size(other.m_size)
    {
        other.m_buffer = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (m_buffer) {
                vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
            }
            m_allocator = other.m_allocator;
            m_buffer = other.m_buffer;
            m_allocation = other.m_allocation;
            m_size = other.m_size;
            other.m_buffer = VK_NULL_HANDLE;
            other.m_allocation = VK_NULL_HANDLE;
        }
        return *this;
    }

    vk::Buffer buffer() const { return m_buffer; }
    vk::DeviceSize size() const { return m_size; }
    VmaAllocation allocation() const { return m_allocation; }

    void* map() {
        void* data;
        vmaMapMemory(m_allocator, m_allocation, &data);
        return data;
    }

    void unmap() {
        vmaUnmapMemory(m_allocator, m_allocation);
    }

    template<typename T>
    void upload(const T* data, size_t count) {
        void* mapped = map();
        std::memcpy(mapped, data, sizeof(T) * count);
        unmap();
    }

    vk::DeviceAddress deviceAddress(vk::Device device) const {
        vk::BufferDeviceAddressInfo info{m_buffer};
        return device.getBufferAddress(info);
    }

private:
    VmaAllocator m_allocator;
    vk::Buffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    vk::DeviceSize m_size = 0;
};
```

**Step 2: Verify it compiles**

Add a temporary include in main.cpp and build:

```bash
cmake --build build 2>&1 | head -20
```

**Step 3: Commit**

```bash
git add src/buffer.hpp
git commit -m "feat: add Buffer utility class for VMA allocations"
```

---

## Task 9: Create Scene Header

**Files:**
- Create: `src/scene.hpp`

**Step 1: Create scene.hpp**

Create `src/scene.hpp`:

```cpp
#pragma once

#include "vulkan_context.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <memory>

struct Vertex {
    glm::vec3 position;
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
```

**Step 2: Commit**

```bash
git add src/scene.hpp
git commit -m "feat: add Scene header with Cornell box structures"
```

---

## Task 10: Implement Scene - Cornell Box Geometry

**Files:**
- Create: `src/scene.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Create scene.cpp with Cornell box geometry**

Create `src/scene.cpp`:

```cpp
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
```

**Step 2: Add scene.cpp to CMakeLists.txt**

Update `CMakeLists.txt` add_executable:

```cmake
add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
    src/scene.cpp
)
```

**Step 3: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

Expected: Compiles successfully.

**Step 4: Commit**

```bash
git add src/scene.cpp CMakeLists.txt
git commit -m "feat: implement Scene with Cornell box geometry"
```

---

## Task 11: Create Ray Tracing Shaders - Common Header

**Files:**
- Create: `shaders/common.glsl`

**Step 1: Create common.glsl with shared definitions**

Create `shaders/common.glsl`:

```glsl
// Shared definitions for all ray tracing shaders

struct RayPayload {
    vec3 color;
    vec3 origin;
    vec3 direction;
    uint seed;
    bool done;
};

struct Material {
    vec3 albedo;
    float pad0;
    vec3 emission;
    float pad1;
};

// PCG random number generator
uint pcg(inout uint state) {
    uint prev = state * 747796405u + 2891336453u;
    uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
    state = prev;
    return (word >> 22u) ^ word;
}

float randomFloat(inout uint seed) {
    return float(pcg(seed)) / float(0xFFFFFFFFu);
}

vec3 randomInHemisphere(vec3 normal, inout uint seed) {
    // Cosine-weighted hemisphere sampling
    float r1 = randomFloat(seed);
    float r2 = randomFloat(seed);

    float phi = 2.0 * 3.14159265359 * r1;
    float cosTheta = sqrt(r2);
    float sinTheta = sqrt(1.0 - r2);

    vec3 w = normal;
    vec3 a = abs(w.x) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 u = normalize(cross(a, w));
    vec3 v = cross(w, u);

    return normalize(u * cos(phi) * sinTheta + v * sin(phi) * sinTheta + w * cosTheta);
}
```

**Step 2: Commit**

```bash
git add shaders/common.glsl
git commit -m "feat: add shared shader definitions"
```

---

## Task 12: Create Ray Generation Shader

**Files:**
- Create: `shaders/raygen.rgen`

**Step 1: Create raygen.rgen**

Create `shaders/raygen.rgen`:

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D outputImage;

layout(push_constant) uniform PushConstants {
    vec3 cameraPosition;
    float fov;
    vec3 cameraForward;
    uint samplesPerPixel;
    vec3 cameraRight;
    uint frameIndex;
    vec3 cameraUp;
    uint maxBounces;
} pc;

layout(location = 0) rayPayloadEXT RayPayload payload;

void main() {
    const uvec2 pixelCoord = gl_LaunchIDEXT.xy;
    const uvec2 imageSize = gl_LaunchSizeEXT.xy;

    uint seed = pixelCoord.x + pixelCoord.y * imageSize.x + pc.frameIndex * imageSize.x * imageSize.y;

    vec3 accumulatedColor = vec3(0.0);

    for (uint s = 0; s < pc.samplesPerPixel; s++) {
        // Jittered pixel position for anti-aliasing
        float jitterX = randomFloat(seed) - 0.5;
        float jitterY = randomFloat(seed) - 0.5;

        vec2 uv = (vec2(pixelCoord) + vec2(0.5) + vec2(jitterX, jitterY)) / vec2(imageSize);
        uv = uv * 2.0 - 1.0; // Convert to [-1, 1]

        float aspect = float(imageSize.x) / float(imageSize.y);
        float tanHalfFov = tan(pc.fov * 0.5);

        vec3 rayDir = normalize(
            pc.cameraForward +
            pc.cameraRight * uv.x * tanHalfFov * aspect +
            pc.cameraUp * uv.y * tanHalfFov
        );

        // Initialize ray
        payload.origin = pc.cameraPosition;
        payload.direction = rayDir;
        payload.color = vec3(0.0);
        payload.seed = seed;
        payload.done = false;

        vec3 throughput = vec3(1.0);
        vec3 color = vec3(0.0);

        for (uint bounce = 0; bounce < pc.maxBounces; bounce++) {
            traceRayEXT(
                topLevelAS,
                gl_RayFlagsOpaqueEXT,
                0xFF,
                0, // sbtRecordOffset
                0, // sbtRecordStride
                0, // missIndex
                payload.origin,
                0.001,
                payload.direction,
                10000.0,
                0  // payload location
            );

            seed = payload.seed;

            if (payload.done) {
                color += throughput * payload.color;
                break;
            }

            throughput *= payload.color;
        }

        accumulatedColor += color;
    }

    vec3 finalColor = accumulatedColor / float(pc.samplesPerPixel);

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);

    imageStore(outputImage, ivec2(pixelCoord), vec4(finalColor, 1.0));
}
```

**Step 2: Commit**

```bash
git add shaders/raygen.rgen
git commit -m "feat: add ray generation shader"
```

---

## Task 13: Create Miss Shader

**Files:**
- Create: `shaders/miss.rmiss`

**Step 1: Create miss.rmiss**

Create `shaders/miss.rmiss`:

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    // No environment lighting - return black
    payload.color = vec3(0.0);
    payload.done = true;
}
```

**Step 2: Commit**

```bash
git add shaders/miss.rmiss
git commit -m "feat: add miss shader"
```

---

## Task 14: Create Closest Hit Shader

**Files:**
- Create: `shaders/closesthit.rchit`

**Step 1: Create closesthit.rchit**

Create `shaders/closesthit.rchit`:

```glsl
#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
hitAttributeEXT vec2 attribs;

struct Vertex {
    vec3 position;
};

layout(binding = 2, set = 0, scalar) readonly buffer Vertices { Vertex vertices[]; };
layout(binding = 3, set = 0, scalar) readonly buffer Indices { uint indices[]; };
layout(binding = 4, set = 0, scalar) readonly buffer Materials { Material materials[]; };
layout(binding = 5, set = 0, scalar) readonly buffer MaterialIndices { uint materialIndices[]; };

void main() {
    // Get triangle vertices
    uint primId = gl_PrimitiveID;
    uint i0 = indices[primId * 3 + 0];
    uint i1 = indices[primId * 3 + 1];
    uint i2 = indices[primId * 3 + 2];

    vec3 v0 = vertices[i0].position;
    vec3 v1 = vertices[i1].position;
    vec3 v2 = vertices[i2].position;

    // Compute hit position using barycentric coordinates
    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec3 hitPos = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;

    // Compute normal (flat shading)
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 normal = normalize(cross(edge1, edge2));

    // Ensure normal faces against ray direction
    if (dot(normal, gl_WorldRayDirectionEXT) > 0.0) {
        normal = -normal;
    }

    // Get material
    uint matIndex = materialIndices[primId];
    Material mat = materials[matIndex];

    // Check if emissive
    if (mat.emission.x > 0.0 || mat.emission.y > 0.0 || mat.emission.z > 0.0) {
        payload.color = mat.emission;
        payload.done = true;
        return;
    }

    // Lambertian diffuse - sample new direction
    vec3 newDir = randomInHemisphere(normal, payload.seed);

    payload.color = mat.albedo;
    payload.origin = hitPos + normal * 0.001;
    payload.direction = newDir;
    payload.done = false;
}
```

**Step 2: Commit**

```bash
git add shaders/closesthit.rchit
git commit -m "feat: add closest hit shader"
```

---

## Task 15: Update CMake for Shader Includes

**Files:**
- Modify: `CMakeLists.txt`

**Step 1: Update shader compilation to handle includes**

The shaders use `#include`, which glslc doesn't handle by default. Update the shader compilation in `CMakeLists.txt`:

Find the shader compilation section and replace with:

```cmake
# Shader compilation
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/raygen.rgen
    ${SHADER_SOURCE_DIR}/miss.rmiss
    ${SHADER_SOURCE_DIR}/closesthit.rchit
)

set(SHADER_INCLUDES
    ${SHADER_SOURCE_DIR}/common.glsl
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND Vulkan::glslc -I${SHADER_SOURCE_DIR} ${SHADER} -o ${SHADER_OUTPUT} --target-env=vulkan1.3
        DEPENDS ${SHADER} ${SHADER_INCLUDES}
        COMMENT "Compiling ${SHADER_NAME}"
    )
    list(APPEND SPV_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_SHADERS})
add_dependencies(pathological shaders)
```

**Step 2: Verify shaders compile**

```bash
cmake --build build 2>&1 | grep -E "(Compiling|error)"
```

Expected: See "Compiling raygen.rgen", "Compiling miss.rmiss", "Compiling closesthit.rchit" with no errors.

**Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "fix: update shader compilation for includes"
```

---

## Task 16: Create PathTracer Header

**Files:**
- Create: `src/path_tracer.hpp`

**Step 1: Create path_tracer.hpp**

Create `src/path_tracer.hpp`:

```cpp
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
};

class PathTracer {
public:
    PathTracer(const VulkanContext& ctx, const Scene& scene,
               uint32_t width, uint32_t height);
    ~PathTracer();

    void render(uint32_t samplesPerPixel);
    void saveImage(const std::string& filename);

private:
    void createOutputImage();
    void createAccelerationStructures();
    void createRayTracingPipeline();
    void createShaderBindingTable();
    void createDescriptorSets();

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
```

**Step 2: Commit**

```bash
git add src/path_tracer.hpp
git commit -m "feat: add PathTracer header"
```

---

## Task 17: Implement PathTracer - Output Image

**Files:**
- Create: `src/path_tracer.cpp`
- Modify: `CMakeLists.txt`

**Step 1: Create path_tracer.cpp with output image creation**

Create `src/path_tracer.cpp`:

```cpp
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
```

**Step 2: Add path_tracer.cpp to CMakeLists.txt**

Update add_executable in `CMakeLists.txt`:

```cmake
add_executable(pathological
    src/main.cpp
    src/vulkan_context.cpp
    src/scene.cpp
    src/path_tracer.cpp
)
```

**Step 3: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

Expected: Compiles with no errors.

**Step 4: Commit**

```bash
git add src/path_tracer.cpp CMakeLists.txt
git commit -m "feat: implement PathTracer output image creation"
```

---

## Task 18: Implement PathTracer - Acceleration Structures

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Implement createAccelerationStructures**

Replace the `createAccelerationStructures` function in `src/path_tracer.cpp`:

```cpp
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
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer acceleration structure building"
```

---

## Task 19: Implement PathTracer - Ray Tracing Pipeline

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Implement createRayTracingPipeline**

Replace the `createRayTracingPipeline` function in `src/path_tracer.cpp`:

```cpp
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
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer ray tracing pipeline"
```

---

## Task 20: Implement PathTracer - Shader Binding Table

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Implement createShaderBindingTable**

Replace the `createShaderBindingTable` function in `src/path_tracer.cpp`:

```cpp
void PathTracer::createShaderBindingTable() {
    const auto& rtProps = m_ctx.rtProperties();
    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;

    uint32_t handleSizeAligned = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);

    uint32_t groupCount = 3; // raygen, miss, hit
    uint32_t sbtSize = groupCount * handleSizeAligned;

    // Get shader group handles
    std::vector<uint8_t> handles(groupCount * handleSize);
    auto result = m_ctx.device().getRayTracingShaderGroupHandlesKHR(
        **m_pipeline, 0, groupCount, handles.size(), handles.data()
    );

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
        std::memcpy(sbtData + i * handleSizeAligned,
                    handles.data() + i * handleSize,
                    handleSize);
    }
    m_sbtBuffer->unmap();

    vk::DeviceAddress sbtAddress = m_sbtBuffer->deviceAddress(*m_ctx.device());

    m_raygenRegion.deviceAddress = sbtAddress;
    m_raygenRegion.stride = handleSizeAligned;
    m_raygenRegion.size = handleSizeAligned;

    m_missRegion.deviceAddress = sbtAddress + handleSizeAligned;
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size = handleSizeAligned;

    m_hitRegion.deviceAddress = sbtAddress + 2 * handleSizeAligned;
    m_hitRegion.stride = handleSizeAligned;
    m_hitRegion.size = handleSizeAligned;

    m_callableRegion = {}; // Not used

    std::cout << "Shader binding table created" << std::endl;
}
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer shader binding table"
```

---

## Task 21: Implement PathTracer - Descriptor Sets

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Implement createDescriptorSets**

Replace the `createDescriptorSets` function in `src/path_tracer.cpp`:

```cpp
void PathTracer::createDescriptorSets() {
    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eAccelerationStructureKHR, 1},
        {vk::DescriptorType::eStorageImage, 1},
        {vk::DescriptorType::eStorageBuffer, 4},
    };

    vk::DescriptorPoolCreateInfo poolInfo{};
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
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer descriptor sets"
```

---

## Task 22: Implement PathTracer - Render Function

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Implement render function**

Replace the `render` function in `src/path_tracer.cpp`:

```cpp
void PathTracer::render(uint32_t samplesPerPixel) {
    std::cout << "Rendering " << m_width << "x" << m_height
              << " with " << samplesPerPixel << " samples per pixel..." << std::endl;

    // Set up camera (looking into Cornell box)
    PushConstants pc{};
    pc.cameraPosition = glm::vec3(0.0f, 1.0f, 3.5f);
    pc.fov = glm::radians(45.0f);

    glm::vec3 target = glm::vec3(0.0f, 1.0f, 0.0f);
    pc.cameraForward = glm::normalize(target - pc.cameraPosition);
    pc.cameraRight = glm::normalize(glm::cross(pc.cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));
    pc.cameraUp = glm::cross(pc.cameraRight, pc.cameraForward);

    pc.samplesPerPixel = samplesPerPixel;
    pc.frameIndex = 0;
    pc.maxBounces = 8;

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
            m_width,
            m_height,
            1
        );
    });

    std::cout << "Rendering complete" << std::endl;
}
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer render function"
```

---

## Task 23: Implement PathTracer - Save Image

**Files:**
- Modify: `src/path_tracer.cpp`

**Step 1: Add stb include at top of file**

Add at the top of `src/path_tracer.cpp`, after other includes:

```cpp
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
```

**Step 2: Implement saveImage function**

Replace the `saveImage` function in `src/path_tracer.cpp`:

```cpp
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
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {m_width, m_height, 1};

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
```

**Step 3: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 4: Commit**

```bash
git add src/path_tracer.cpp
git commit -m "feat: implement PathTracer image saving with stb"
```

---

## Task 24: Implement Main with CLI11

**Files:**
- Modify: `src/main.cpp`

**Step 1: Update main.cpp with full implementation**

Replace the contents of `src/main.cpp`:

```cpp
#include "vulkan_context.hpp"
#include "scene.hpp"
#include "path_tracer.hpp"

#include <CLI/CLI.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CLI::App app{"Pathological - Vulkan Path Tracer"};

    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t samples = 256;
    std::string output = "output.png";

    app.add_option("-W,--width", width, "Image width")->default_val(1024);
    app.add_option("-H,--height", height, "Image height")->default_val(1024);
    app.add_option("-s,--samples", samples, "Samples per pixel")->default_val(256);
    app.add_option("-o,--output", output, "Output filename")->default_val("output.png");

    CLI11_PARSE(app, argc, argv);

    try {
        std::cout << "Pathological - Vulkan Path Tracer" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Resolution: " << width << "x" << height << std::endl;
        std::cout << "Samples: " << samples << std::endl;
        std::cout << "Output: " << output << std::endl;
        std::cout << std::endl;

        VulkanContext ctx;
        Scene scene(ctx);
        PathTracer tracer(ctx, scene, width, height);

        tracer.render(samples);
        tracer.saveImage(output);

        std::cout << std::endl;
        std::cout << "Done!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

**Step 2: Verify it compiles**

```bash
cmake --build build 2>&1 | head -30
```

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: implement main with CLI11 argument parsing"
```

---

## Task 25: Build and Test

**Step 1: Full rebuild**

```bash
rm -rf build && cmake --preset default && cmake --build build
```

Expected: Build succeeds with all shaders and source files.

**Step 2: Run the path tracer**

```bash
cd build && ./pathological -W 512 -H 512 -s 64 -o test.png
```

Expected: Renders Cornell box to test.png.

**Step 3: Verify output exists**

```bash
ls -la build/test.png
```

Expected: test.png file exists with reasonable size.

**Step 4: Commit final state**

```bash
git add -A && git commit -m "chore: complete MVP path tracer implementation"
```

---

## Task 26: Update README

**Files:**
- Modify: `README.md`

**Step 1: Update README with usage instructions**

Update `README.md` to include:

```markdown
# Pathological

Pathological is a Vulkan 1.3 path tracer using hardware ray tracing.

## Features

- Hardware-accelerated ray tracing via Vulkan RT extensions
- Cornell box scene with emissive and Lambertian materials
- Headless rendering (no window required)
- PNG output

## Requirements

- Vulkan SDK 1.3+
- NVIDIA GPU with ray tracing support
- vcpkg
- CMake 3.20+

## Building

```bash
cmake --preset default
cmake --build build
```

## Usage

```bash
./build/pathological [options]

Options:
  -W, --width   Image width (default: 1024)
  -H, --height  Image height (default: 1024)
  -s, --samples Samples per pixel (default: 256)
  -o, --output  Output filename (default: output.png)
```

## Example

```bash
./build/pathological -W 1920 -H 1080 -s 512 -o render.png
```
```

**Step 2: Commit**

```bash
git add README.md
git commit -m "docs: update README with build and usage instructions"
```
