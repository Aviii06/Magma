#include <magma_engine/core/renderer/RenderResourceAllocator.h>
#include <magma_engine/core/renderer/VkInitializers.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <logging/Logger.h>
#include <cassert>

namespace Magma
{
    void RenderResourceAllocator::Initialize(VkDevice device, VmaAllocator allocator)
    {
        assert(device != VK_NULL_HANDLE && "RenderResourceAllocator::Initialize() - VkDevice is null!");
        assert(allocator != VK_NULL_HANDLE && "RenderResourceAllocator::Initialize() - VmaAllocator is null!");

        m_device = device;
        m_allocator = allocator;

        m_descriptorManager = std::make_shared<DescriptorManager>();
        m_descriptorManager->Init(m_device);

        m_initialized = true;
        Logger::Log(LogLevel::INFO, "RenderResourceAllocator initialized");
    }

    void RenderResourceAllocator::AllocateImages(
        const Map<String, BufferRequirement>& requirements,
        VkExtent2D extent)
    {
        assert(m_initialized && "RenderResourceAllocator::AllocateImages() - Not initialized! Call Initialize(device, allocator) first.");

        Logger::Log(LogLevel::DEBUG, "Allocating {} images", requirements.size());

        for (const auto& [name, req] : requirements)
        {
            VkExtent2D imageExtent = req.matchSwapchainExtent ? extent : req.extent;

            AllocatedImage image = CreateImage(req.format, req.usage, imageExtent);
            auto imagePtr = std::make_shared<AllocatedImage>(image);

            m_allocatedImages[name] = imagePtr;
            m_bufferRegistry.RegisterBuffer(name, imagePtr);

            Logger::Log(LogLevel::DEBUG, "  Allocated image '{}' ({}x{}, format: {})",
                name, imageExtent.width, imageExtent.height, static_cast<uint32_t>(req.format));
        }

        Logger::Log(LogLevel::DEBUG, "Image allocation complete");
    }

    void RenderResourceAllocator::DeallocateImages()
    {
        assert(m_initialized && "RenderResourceAllocator::DeallocateImages() - Not initialized!");

        Logger::Log(LogLevel::DEBUG, "Deallocating {} images", m_allocatedImages.size());

        for (auto& [name, imagePtr] : m_allocatedImages)
        {
            if (imagePtr)
            {
                DestroyImage(std::move(imagePtr));
            }
        }

        m_allocatedImages.clear();
        m_bufferRegistry.Clear();
    }

    std::shared_ptr<AllocatedImage> RenderResourceAllocator::GetImage(const String& name) const
    {
        assert(m_initialized && "RenderResourceAllocator::GetImage() - Not initialized!");
        return m_bufferRegistry.GetBuffer(name);
    }

    const Map<String, std::shared_ptr<AllocatedImage>>&
    RenderResourceAllocator::GetAllImages() const
    {
        assert(m_initialized && "RenderResourceAllocator::GetAllImages() - Not initialized!");
        return m_allocatedImages;
    }

    std::shared_ptr<DescriptorManager> RenderResourceAllocator::GetDescriptorManager() const
    {
        assert(m_initialized && "RenderResourceAllocator::GetDescriptorManager() - Not initialized!");
        return m_descriptorManager;
    }

    VkDevice RenderResourceAllocator::GetDevice() const
    {
        assert(m_initialized && "RenderResourceAllocator::GetDevice() - Not initialized!");
        return m_device;
    }

    VmaAllocator RenderResourceAllocator::GetAllocator() const
    {
        assert(m_initialized && "RenderResourceAllocator::GetAllocator() - Not initialized!");
        return m_allocator;
    }

    BufferRegistry& RenderResourceAllocator::GetBufferRegistry()
    {
        assert(m_initialized && "RenderResourceAllocator::GetBufferRegistry() - Not initialized!");
        return m_bufferRegistry;
    }

    const BufferRegistry& RenderResourceAllocator::GetBufferRegistry() const
    {
        assert(m_initialized && "RenderResourceAllocator::GetBufferRegistry() - Not initialized!");
        return m_bufferRegistry;
    }

    void RenderResourceAllocator::Cleanup()
    {
        if (!m_initialized)
        {
            return;
        }

        DeallocateImages();

        if (m_descriptorManager)
        {
            m_descriptorManager->Cleanup();
            m_descriptorManager.reset();
        }

        m_device = VK_NULL_HANDLE;
        m_allocator = VK_NULL_HANDLE;
        m_initialized = false;

        Logger::Log(LogLevel::INFO, "RenderResourceAllocator cleaned up");
    }

    AllocatedImage RenderResourceAllocator::CreateImage(
        VkFormat format,
        VkImageUsageFlags usage,
        VkExtent2D extent)
    {
        assert(m_initialized && "RenderResourceAllocator::CreateImage() - Not initialized!");

        AllocatedImage image{};

        VkExtent3D imageExtent = {
            extent.width,
            extent.height,
            1
        };

        image.imageFormat = format;
        image.imageExtent = imageExtent;

        VkImageCreateInfo imgInfo = vkinit::image_create_info(format, usage, imageExtent);

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(m_allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

        VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(format, image.image, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &image.imageView));

        return image;
    }

    void RenderResourceAllocator::DestroyImage(std::shared_ptr<AllocatedImage> image)
    {
        assert(m_initialized && "RenderResourceAllocator::DestroyImage() - Not initialized!");

        if (image->imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device, image->imageView, nullptr);
            image->imageView = VK_NULL_HANDLE;
        }

        if (image->image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_allocator, image->image, image->allocation);
            image->image = VK_NULL_HANDLE;
            image->allocation = VK_NULL_HANDLE;
        }

        image.reset();
    }
}
