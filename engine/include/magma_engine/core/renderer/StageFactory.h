#pragma once

#include <magma_engine/core/renderer/RenderStage.h>
#include <magma_engine/core/renderer/StageConfiguration.h>
#include <memory>

namespace Magma
{
    class StageFactory
    {
    public:
        static std::unique_ptr<RenderStage> CreateComputeStage(
            const String& stageName,
            const String& shaderPath,
            const String& outputBufferName,
            uint32_t workgroupSizeX = 16,
            uint32_t workgroupSizeY = 16);

        static std::unique_ptr<RenderStage> CreateComputeStageAdvanced(
            const String& stageName,
            const String& shaderPath,
            const Vector<BufferBinding>& inputs,
            const Vector<BufferBinding>& outputs,
            const ComputeConfig& computeConfig = ComputeConfig{});

        static std::unique_ptr<RenderStage> CreateGraphicsStage(
            const String& stageName,
            const String& vertexShaderPath,
            const String& fragmentShaderPath,
            const String& outputBufferName,
            VkFormat colorFormat = VK_FORMAT_R16G16B16A16_SFLOAT);

        static std::unique_ptr<RenderStage> CreateFromConfiguration(
            const StageConfiguration& config);
    };
}

