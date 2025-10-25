#include <magma_engine/core/renderer/Pipeline.h>
#include <logging/Logger.h>

namespace Magma
{
    Pipeline::Pipeline(Pipeline&& other) noexcept
        : m_device(other.m_device)
        , m_pipeline(other.m_pipeline)
        , m_pipelineLayout(other.m_pipelineLayout)
    {
        other.m_device = VK_NULL_HANDLE;
        other.m_pipeline = VK_NULL_HANDLE;
        other.m_pipelineLayout = VK_NULL_HANDLE;
    }

    Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();

            m_device = other.m_device;
            m_pipeline = other.m_pipeline;
            m_pipelineLayout = other.m_pipelineLayout;

            other.m_device = VK_NULL_HANDLE;
            other.m_pipeline = VK_NULL_HANDLE;
            other.m_pipelineLayout = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool Pipeline::CreatePipelineLayout(const PipelineLayoutInfo& layoutInfo)
    {
        VkPipelineLayoutCreateInfo layoutCreateInfo{};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = nullptr;
        layoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layoutInfo.descriptorSetLayouts.size());
        layoutCreateInfo.pSetLayouts = layoutInfo.descriptorSetLayouts.data();
        layoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(layoutInfo.pushConstantRanges.size());
        layoutCreateInfo.pPushConstantRanges = layoutInfo.pushConstantRanges.data();

        if (vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        {
            Logger::Log(LogLevel::ERROR, "Failed to create pipeline layout");
            return false;
        }

        return true;
    }

    void Pipeline::Destroy()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            if (m_pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_device, m_pipeline, nullptr);
                m_pipeline = VK_NULL_HANDLE;
            }

            if (m_pipelineLayout != VK_NULL_HANDLE)
            {
                vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
                m_pipelineLayout = VK_NULL_HANDLE;
            }

            m_device = VK_NULL_HANDLE;
        }
    }
}

