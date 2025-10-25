#pragma once
#include <types/Containers.h>
#include <maths/Vec.h>

#include <magma_engine/ServiceLocater.h>
#include <magma_engine/core/renderer/Image.h>
#include <magma_engine/core/renderer/DeletionQueue.h>
#include <magma_engine/core/renderer/ShaderModule.h>
#include <magma_engine/core/renderer/RenderResourceAllocator.h>
#include <magma_engine/core/renderer/RenderOrchestrator.h>

const int FRAME_OVERLAP = 3;

namespace Magma
{
    struct FrameData
    {
        VkCommandPool m_commandPool;
        VkCommandBuffer m_mainCommandBuffer;
        VkSemaphore m_swapchainSemaphore, m_renderSemaphore;
        VkFence m_renderFence;
        DeletionQueue m_deletionQueue;
    };

    struct ImmRenderData
    {
        VkFence immFence;
        VkCommandBuffer immCommandBuffer;
        VkCommandPool immCommandPool;
    };

    class Renderer : public IService
    {
    public:
        void Init();
        void Cleanup() override;
        FrameData& get_current_frame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; };

        void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

        void BeginFrame();
        void RenderScene();
        void CopyToSwapchain();
        void BeginUIRenderPass();
        void EndFrame();
        void Present();
        VkCommandBuffer GetCurrentCommandBuffer() { return get_current_frame().m_mainCommandBuffer; }
        uint32_t GetCurrentSwapchainIndex() const { return m_currentSwapchainImageIndex; }

        // ImGui / Editor Integration
        VkDevice GetDevice() const { return m_device; }
        VkInstance GetInstance() const { return m_instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
        VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
        uint32_t GetGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
        VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
        std::shared_ptr<AllocatedImage> GetDrawImage() { return m_renderOrchestrator.GetBuffer("drawImage"); }
        VkExtent2D GetDrawExtent() const { return m_drawExtent; }
        VkSampler GetDrawImageSampler() const { return m_drawImageSampler; }

    private:
        void init_vulkan();
        void init_swapchain();
        void init_commands();
        void init_sync_structures();
        void init_descriptors();
        void init_render_stages();

        void create_swapchain(Maths::Vec2<uint32_t> size);
        void destroy_swapchain();

    private:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;
        VkSurfaceKHR m_surface;

        VkSwapchainKHR m_swapchain;
        VkFormat m_swapchainImageFormat;
        Vector<VkImage> m_swapchainImages;
        Vector<VkImageView> m_swapchainImageViews;
        VkExtent2D m_swapchainExtent;

        Vector<FrameData> m_frames;
        uint32_t m_frameNumber {0};
        VkQueue m_graphicsQueue;
        uint32_t m_graphicsQueueFamily;

        VkExtent2D m_drawExtent;

        VmaAllocator m_allocator;

        DeletionQueue m_mainDeletionQueue;

        ImmRenderData m_immRenderData;

        std::shared_ptr<RenderResourceAllocator> m_resourceAllocator;
        RenderOrchestrator m_renderOrchestrator;

        VkSampler m_drawImageSampler;

        PFN_vkCmdBeginRenderingKHR m_vkCmdBeginRenderingKHR = nullptr;
        PFN_vkCmdEndRenderingKHR m_vkCmdEndRenderingKHR = nullptr;
        PFN_vkCmdBlitImage2KHR m_vkCmdBlitImage2 = nullptr;

        uint32_t m_currentSwapchainImageIndex = 0;
    };
}
