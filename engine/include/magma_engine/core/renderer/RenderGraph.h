#pragma once
#include <memory>

#include <types/Containers.h>
#include <types/VkTypes.h>
#include <magma_engine/core/renderer/RenderStage.h>

namespace Magma
{
    struct StageConnection
    {
        String fromStage;
        String toStage;
        String bufferName;
    };

    class RenderGraph
    {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        void AddStage(std::unique_ptr<RenderStage> stage);
        void RemoveStage(const String& stageName);
        void ClearStages();

        void ConnectStages(const String& fromStage, const String& toStage, const String& bufferName);
        Vector<StageConnection> GetConnections() const;

        Vector<String> GetExecutionOrder() const;
        size_t GetStageCount() const { return m_executionOrder.size(); }

        Vector<BufferRequirement> CollectAllBufferRequirements() const;
        Map<String, BufferRequirement> CollectUniqueBufferRequirements() const;

        void Cleanup();
        void OnResolutionChanged(VkExtent2D newExtent);

        String GetFinalOutputBufferName() const;

        // TODO: Implement more advanced iteration (e.g., DAG support)
        class StageIterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = RenderStage*;
            using difference_type = std::ptrdiff_t;
            using pointer = RenderStage**;
            using reference = RenderStage*&;

            StageIterator(Vector<String>::const_iterator orderIt,
                         const Map<String, std::unique_ptr<RenderStage>>* stages)
                : m_orderIt(orderIt), m_stages(stages) {}

            RenderStage* operator*() const {
                auto it = m_stages->find(*m_orderIt);
                return it != m_stages->end() ? it->second.get() : nullptr;
            }

            StageIterator& operator++() {
                ++m_orderIt;
                return *this;
            }

            bool operator!=(const StageIterator& other) const {
                return m_orderIt != other.m_orderIt;
            }

        private:
            Vector<String>::const_iterator m_orderIt;
            const Map<String, std::unique_ptr<RenderStage>>* m_stages;
        };

        StageIterator begin() { return StageIterator(m_executionOrder.begin(), &m_stages); }
        StageIterator end() { return StageIterator(m_executionOrder.end(), &m_stages); }

    private:
        void RebuildExecutionOrder();

    private:
        Map<String, std::unique_ptr<RenderStage>> m_stages;
        Vector<String> m_executionOrder;
        Vector<StageConnection> m_connections;
    };
}
