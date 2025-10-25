#pragma once

#include <types/Containers.h>
#include <types/VkTypes.h>
#include <vma/vk_mem_alloc.h>
#include <magma_engine/core/renderer/RenderStage.h>
#include <magma_engine/core/renderer/RenderGraph.h>
#include <magma_engine/core/renderer/RenderResourceAllocator.h>
#include <memory>

namespace Magma
{
    class RenderOrchestrator
    {
    public:
        RenderOrchestrator() = default;
        ~RenderOrchestrator() = default;

        void AddStage(std::unique_ptr<RenderStage> stage);

        void Initialize(
            std::shared_ptr<RenderResourceAllocator> resourceAllocator,
            VkExtent2D swapchainExtent
        );

        void Execute(VkCommandBuffer cmd);

        void Cleanup();

        std::shared_ptr<AllocatedImage> GetBuffer(const String& name) const;

        std::shared_ptr<AllocatedImage> GetFinalOutputBuffer() const;

        void OnResolutionChanged(VkExtent2D newExtent);

        // Access to the render graph for advanced configuration
        RenderGraph& GetRenderGraph() { return m_renderGraph; }
        const RenderGraph& GetRenderGraph() const { return m_renderGraph; }

    private:
        void CollectBufferRequirements();
        void AllocateBuffers();
        void DeallocateBuffers();

    private:
        RenderGraph m_renderGraph;
        std::weak_ptr<RenderResourceAllocator> m_resourceAllocator;
        Map<String, BufferRequirement> m_bufferRequirements;

        VkExtent2D m_currentExtent = {0, 0};
        bool m_initialized = false;
    };
}
