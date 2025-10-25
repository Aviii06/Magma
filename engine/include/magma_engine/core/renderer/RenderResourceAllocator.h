#pragma once

#include <memory>
#include <types/Containers.h>
#include <types/VkTypes.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <magma_engine/core/renderer/Image.h>
#include <magma_engine/core/renderer/BufferRegistry.h>
#include <magma_engine/core/renderer/DescriptorManager.h>
#include <magma_engine/core/renderer/RenderStage.h>

namespace Magma
{
    class RenderResourceAllocator
    {
    public:
        RenderResourceAllocator() = default;
        ~RenderResourceAllocator() = default;

        void Initialize(VkDevice device, VmaAllocator allocator);

        void AllocateImages(const Map<String, BufferRequirement>& requirements, VkExtent2D extent);
        void DeallocateImages();

        std::shared_ptr<AllocatedImage> GetImage(const String& name) const;
        const Map<String, std::shared_ptr<AllocatedImage>>& GetAllImages() const;

        std::shared_ptr<DescriptorManager> GetDescriptorManager() const;

        VkDevice GetDevice() const;
        VmaAllocator GetAllocator() const;

        BufferRegistry& GetBufferRegistry();
        const BufferRegistry& GetBufferRegistry() const;

        void Cleanup();

        bool IsInitialized() const { return m_initialized; }

    private:
        bool m_initialized = false;
        VkDevice m_device = VK_NULL_HANDLE;
        VmaAllocator m_allocator = VK_NULL_HANDLE;

        std::shared_ptr<DescriptorManager> m_descriptorManager;
        BufferRegistry m_bufferRegistry;
        Map<String, std::shared_ptr<AllocatedImage>> m_allocatedImages;

        AllocatedImage CreateImage(VkFormat format, VkImageUsageFlags usage, VkExtent2D extent);
        void DestroyImage(std::shared_ptr<AllocatedImage> image);
    };
}
