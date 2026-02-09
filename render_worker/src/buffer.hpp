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
