#pragma once
#include <types/VkTypes.h>
#include <types/Containers.h>

namespace Magma
{
    struct PipelineLayoutInfo
    {
        Vector<VkDescriptorSetLayout> descriptorSetLayouts;
        Vector<VkPushConstantRange> pushConstantRanges;
    };

    class Pipeline
    {
    protected:
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

        bool CreatePipelineLayout(const PipelineLayoutInfo& layoutInfo);

        Pipeline() = default;

    public:
        virtual ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        VkPipeline GetPipeline() const { return m_pipeline; }
        VkPipelineLayout GetLayout() const { return m_pipelineLayout; }
        bool IsValid() const { return m_pipeline != VK_NULL_HANDLE; }

        virtual void Bind(VkCommandBuffer cmd) const = 0;

        virtual void Destroy();
    };
}

