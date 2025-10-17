#pragma once
#include <types/VkTypes.h>
#include <types/Containers.h>
#include <maths/Vec.h>
#include <magma_engine/ServiceLocater.h>
#include <magma_engine/core/renderer/Pipeline.h>
#include <magma_engine/core/renderer/ShaderModule.h>
#include <magma_engine/core/renderer/DescriptorManager.h>

const int FRAME_OVERLAP = 3;

namespace Magma
{
    struct FrameData
    {
        VkCommandPool m_commandPool;
        VkCommandBuffer m_mainCommandBuffer;
        VkSemaphore m_swapchainSemaphore, m_renderSemaphore;
        VkFence m_renderFence;
    };

    class Renderer : public IService
    {
    public:
        void Init();
        void Draw();
        void Cleanup() override;
        FrameData& get_current_frame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; };

    private:
        void init_vulkan();
        void init_swapchain();
        void init_commands();
        void init_sync_structures();
        void init_pipelines();
        void init_background_pipelines();

        void draw_background(VkCommandBuffer cmd);
        void draw_geometry(VkCommandBuffer cmd, uint32_t swapchainImageIndex);

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

        // Pipeline and shader resources
        DescriptorManager m_descriptorManager;

        // TODO: Move these to seperate render stages, each render stage contain there own shaders and pipelines.
        Pipeline m_trianglePipeline;
        ShaderModule m_triangleVertShader;
        ShaderModule m_triangleFragShader;

        // Dynamic rendering function pointers (for MoltenVK compatibility)
        PFN_vkCmdBeginRenderingKHR m_vkCmdBeginRenderingKHR = nullptr;
        PFN_vkCmdEndRenderingKHR m_vkCmdEndRenderingKHR = nullptr;
    };
}
