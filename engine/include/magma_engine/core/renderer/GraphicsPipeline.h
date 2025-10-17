#pragma once

#include <magma_engine/core/renderer/Pipeline.h>

namespace Magma
{
    class GraphicsPipeline : public Pipeline
    {
    public:
        GraphicsPipeline() = default;
        ~GraphicsPipeline() override = default;

        bool Create(
            VkDevice device,
            const PipelineLayoutInfo& layoutInfo,
            VkShaderModule vertexShader,
            VkShaderModule fragmentShader,
            VkFormat colorAttachmentFormat,
            VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED
        );

        void Bind(VkCommandBuffer cmd) const override;
    };
}
