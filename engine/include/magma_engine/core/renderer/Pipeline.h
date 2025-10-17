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
    public:
        Pipeline() = default;
        ~Pipeline();

        // Delete copy operations
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        // Move operations
        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        bool CreateGraphicsPipeline(
            VkDevice device,
            const PipelineLayoutInfo& layoutInfo,
            VkShaderModule vertexShader,
            VkShaderModule fragmentShader,
            VkFormat colorAttachmentFormat,
            VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED
        );

        void Destroy();

        VkPipeline GetPipeline() const { return m_pipeline; }
        VkPipelineLayout GetLayout() const { return m_pipelineLayout; }
        bool IsValid() const { return m_pipeline != VK_NULL_HANDLE; }

        void Bind(VkCommandBuffer cmd) const;

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    };
}

