#include <magma_engine/core/renderer/RenderGraph.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <logging/Logger.h>

namespace Magma
{
    void RenderGraph::AddStage(std::unique_ptr<RenderStage> stage)
    {
        String stageName = stage->GetStageName();
        m_stages[stageName] = std::move(stage);

    	// TODO: In future have explicit ordering and DAG support.
        RebuildExecutionOrder();

        Logger::Log(LogLevel::INFO, "[RenderGraph] Added stage: {}", stageName);
    }

    void RenderGraph::RemoveStage(const String& stageName)
    {
        auto it = m_stages.find(stageName);
        if (it != m_stages.end())
        {
            m_stages.erase(it);
            RebuildExecutionOrder();
            Logger::Log(LogLevel::INFO, "[RenderGraph] Removed stage: {}", stageName);
        }
    }

    void RenderGraph::ClearStages()
    {
        m_stages.clear();
        m_executionOrder.clear();
        m_connections.clear();
        Logger::Log(LogLevel::INFO, "[RenderGraph] Cleared all stages");
    }

    void RenderGraph::ConnectStages(const String& fromStage, const String& toStage, const String& bufferName)
    {
        // For future DAG support
        StageConnection connection{fromStage, toStage, bufferName};
        m_connections.push_back(connection);
        Logger::Log(LogLevel::DEBUG, "[RenderGraph] Connected stage '{}' to '{}' via buffer '{}'",
            fromStage, toStage, bufferName);
    }

    Vector<StageConnection> RenderGraph::GetConnections() const
    {
        return m_connections;
    }

    Vector<String> RenderGraph::GetExecutionOrder() const
    {
        return m_executionOrder;
    }

    Vector<BufferRequirement> RenderGraph::CollectAllBufferRequirements() const
    {
        Vector<BufferRequirement> allRequirements;

        for (const auto& stageName : m_executionOrder)
        {
            auto it = m_stages.find(stageName);
            if (it != m_stages.end())
            {
                auto stageReqs = it->second->GetBufferRequirements();
                allRequirements.insert(allRequirements.end(), stageReqs.begin(), stageReqs.end());
            }
        }

        return allRequirements;
    }

    Map<String, BufferRequirement> RenderGraph::CollectUniqueBufferRequirements() const
    {
        Map<String, BufferRequirement> uniqueRequirements;

        for (const auto& stageName : m_executionOrder)
        {
            auto it = m_stages.find(stageName);
            if (it == m_stages.end()) continue;

            auto requirements = it->second->GetBufferRequirements();

            for (const auto& req : requirements)
            {
                if (uniqueRequirements.find(req.name) == uniqueRequirements.end())
                {
                    uniqueRequirements[req.name] = req;
                    Logger::Log(LogLevel::DEBUG, "[RenderGraph] Stage '{}' requires buffer '{}'",
                        stageName, req.name);
                }
                else
                {
                    // Buffer already exists - validate it matches
                    const auto& existing = uniqueRequirements[req.name];

                    if (existing.format != req.format)
                    {
                        Logger::Log(LogLevel::WARNING,
                            "[RenderGraph] Stage '{}' requires buffer '{}' with different format. Existing: {}, Requested: {}",
                            stageName, req.name, static_cast<uint32_t>(existing.format), static_cast<uint32_t>(req.format));
                    }

                    // Merge usage flags (allow aliasing with different uses)
                    uniqueRequirements[req.name].usage |= req.usage;
                }
            }
        }

        Logger::Log(LogLevel::DEBUG, "[RenderGraph] Collected {} unique buffer requirements", uniqueRequirements.size());
        return uniqueRequirements;
    }

    void RenderGraph::Cleanup()
    {
        Logger::Log(LogLevel::INFO, "[RenderGraph] Cleaning up");

        for (auto& [name, stage] : m_stages)
        {
            stage->Cleanup();
        }
    }

    void RenderGraph::OnResolutionChanged(VkExtent2D newExtent)
    {
        Logger::Log(LogLevel::INFO, "[RenderGraph] Resolution changed to {}x{}", newExtent.width, newExtent.height);

        for (auto& [name, stage] : m_stages)
        {
            stage->OnResolutionChanged(newExtent);
        }
    }

    String RenderGraph::GetFinalOutputBufferName() const
    {
        const String& lastStageName = m_executionOrder.back();
        auto it = m_stages.find(lastStageName);

        if (it == m_stages.end())
        {
            return "";
        }

        auto requirements = it->second->GetBufferRequirements();
        for (const auto& req : requirements)
        {
            if (req.isOutput)
            {
                return req.name;
            }
        }

        return "";
    }

    void RenderGraph::RebuildExecutionOrder()
    {
        // For linear execution, maintain insertion order
        m_executionOrder.clear();

        for (const auto& [name, stage] : m_stages)
        {
            m_executionOrder.push_back(name);
        }

        Logger::Log(LogLevel::DEBUG, "[RenderGraph] Rebuilt execution order with {} stages", m_executionOrder.size());
    }
}
