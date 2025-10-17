#include <magma_engine/core/renderer/ComputePipeline.h>
#include <logging/Logging.h>

namespace Magma
{
    bool ComputePipeline::Create(
        VkDevice device,
        const PipelineLayoutInfo& layoutInfo,
        VkShaderModule computeShader)
    {
        m_device = device;

        // Create pipeline layout using base class helper
        if (!CreatePipelineLayout(layoutInfo))
        {
            return false;
        }

        // Create compute pipeline
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.pNext = nullptr;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShader;
        shaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = m_pipelineLayout;

        if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            Logger::Log(LogLevel::ERROR, "Failed to create compute pipeline");
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
            return false;
        }

        return true;
    }

    void ComputePipeline::Bind(VkCommandBuffer cmd) const
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    }

    void ComputePipeline::Dispatch(VkCommandBuffer cmd, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const
    {
        vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
    }
}
