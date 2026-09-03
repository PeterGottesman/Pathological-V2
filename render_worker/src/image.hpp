#pragma once

#include <vk_mem_alloc.h>

#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

// RAII wrapper around a VMA-backed vk::Image, mirroring Buffer's shape.
// Throws on creation failure; frees the image in the destructor so that
// a throw partway through a larger construction (e.g. PathTracer's) cannot
// leak the underlying VMA allocation.
class Image {
public:
    Image(VmaAllocator allocator, const VkImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo)
        : m_allocator(allocator) {
        VkImage image;
        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &m_allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image");
        }
        m_image = image;
    }

    ~Image() {
        if (m_image) {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
        }
    }

    // Non-copyable, movable
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept
        : m_allocator(other.m_allocator), m_image(other.m_image), m_allocation(other.m_allocation) {
        other.m_image = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
    }

    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            if (m_image) {
                vmaDestroyImage(m_allocator, m_image, m_allocation);
            }
            m_allocator = other.m_allocator;
            m_image = other.m_image;
            m_allocation = other.m_allocation;
            other.m_image = VK_NULL_HANDLE;
            other.m_allocation = VK_NULL_HANDLE;
        }
        return *this;
    }

    vk::Image image() const { return m_image; }
    VmaAllocation allocation() const { return m_allocation; }

private:
    VmaAllocator m_allocator;
    vk::Image m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
};
