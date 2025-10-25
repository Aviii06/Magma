#include <stdint.h>
#include <magma_engine/core/renderer/RenderOrchestrator.h>
#include <logging/Logger.h>
#include <cassert>

namespace Magma
{
    void RenderOrchestrator::AddStage(std::unique_ptr<RenderStage> stage)
    {
        if (m_initialized)
        {
            Logger::Log(LogLevel::ERROR, "Cannot add stage '{}' after orchestrator is initialized", stage->GetStageName());
            return;
        }

        m_renderGraph.AddStage(std::move(stage));
    }

    void RenderOrchestrator::Initialize(
        std::shared_ptr<RenderResourceAllocator> resourceAllocator,
        VkExtent2D swapchainExtent)
    {
        assert(resourceAllocator && "RenderOrchestrator::Initialize() - resourceAllocator is null!");
        assert(resourceAllocator->IsInitialized() && "RenderOrchestrator::Initialize() - RenderResourceAllocator not initialized! Call resourceAllocator->Initialize() first.");

        if (m_initialized)
        {
            Logger::Log(LogLevel::WARNING, "RenderOrchestrator already initialized");
            return;
        }

        m_resourceAllocator = resourceAllocator;
        m_currentExtent = swapchainExtent;

        Logger::Log(LogLevel::INFO, "Initializing RenderOrchestrator with {} stage(s)",
            m_renderGraph.GetStageCount());

        // Collect requirements and allocate through resource allocator
        CollectBufferRequirements();
        AllocateBuffers();

        // Initialize each stage - pass allocator directly
        for (auto* stage : m_renderGraph) {
            stage->Initialize(
                resourceAllocator->GetDevice(),
                resourceAllocator->GetBufferRegistry(),
                resourceAllocator->GetDescriptorManager());
        }

        m_initialized = true;
        Logger::Log(LogLevel::INFO, "RenderOrchestrator initialization complete");
    }

    void RenderOrchestrator::Execute(VkCommandBuffer cmd)
    {
        assert(m_initialized && "RenderOrchestrator::Execute() - Not initialized! Call Initialize() first.");

        auto allocator = m_resourceAllocator.lock();
        if (!allocator)
        {
            Logger::Log(LogLevel::ERROR, "Resource allocator no longer available");
            return;
        }

        assert(allocator->IsInitialized() && "RenderOrchestrator::Execute() - RenderResourceAllocator no longer initialized!");

        // Execute each stage
        for (auto* stage : m_renderGraph)
        {
            stage->Execute(cmd);
        }
    }

    void RenderOrchestrator::Cleanup()
    {
        if (!m_initialized)
        {
            return;
        }

        Logger::Log(LogLevel::INFO, "Cleaning up RenderOrchestrator");

        auto allocator = m_resourceAllocator.lock();
        if (allocator)
        {
            m_renderGraph.Cleanup();
            DeallocateBuffers();
        }

        m_bufferRequirements.clear();
        m_initialized = false;
    }

    std::shared_ptr<AllocatedImage> RenderOrchestrator::GetBuffer(const String& name) const
    {
        auto allocator = m_resourceAllocator.lock();
        return allocator ? allocator->GetImage(name) : nullptr;
    }

    std::shared_ptr<AllocatedImage> RenderOrchestrator::GetFinalOutputBuffer() const
    {
        String finalBufferName = m_renderGraph.GetFinalOutputBufferName();
        if (finalBufferName.empty())
        {
            Logger::Log(LogLevel::ERROR, "No final output buffer defined");
            return nullptr;
        }

        return GetBuffer(finalBufferName);
    }

    void RenderOrchestrator::OnResolutionChanged(VkExtent2D newExtent)
    {
        assert(m_initialized && "RenderOrchestrator::OnResolutionChanged() - Not initialized!");

        Logger::Log(LogLevel::INFO, "Resolution changed to {}x{}, recreating buffers", newExtent.width, newExtent.height);

        m_currentExtent = newExtent;

        DeallocateBuffers();
        AllocateBuffers();

        m_renderGraph.OnResolutionChanged(newExtent);
    }

    void RenderOrchestrator::CollectBufferRequirements()
    {
        Logger::Log(LogLevel::DEBUG, "Collecting buffer requirements from render graph");
        m_bufferRequirements = m_renderGraph.CollectUniqueBufferRequirements();
        Logger::Log(LogLevel::DEBUG, "Collected {} unique buffer requirements", m_bufferRequirements.size());
    }

    void RenderOrchestrator::AllocateBuffers()
    {
        auto allocator = m_resourceAllocator.lock();
        if (allocator)
        {
            allocator->AllocateImages(m_bufferRequirements, m_currentExtent);
        }
    }

    void RenderOrchestrator::DeallocateBuffers()
    {
        auto allocator = m_resourceAllocator.lock();
        if (allocator)
        {
            allocator->DeallocateImages();
        }
    }
}
