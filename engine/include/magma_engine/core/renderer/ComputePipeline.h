#pragma once

#include <magma_engine/core/renderer/Pipeline.h>

namespace Magma
{
    class ComputePipeline : public Pipeline
    {
    public:
        ComputePipeline() = default;

        bool Create(
            VkDevice device,
            const PipelineLayoutInfo& layoutInfo,
            VkShaderModule computeShader
        );

        void Bind(VkCommandBuffer cmd) const override;

        // Compute-specific functionality
        void Dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;
    };
}
