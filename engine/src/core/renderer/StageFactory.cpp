#include <magma_engine/core/renderer/StageFactory.h>

namespace Magma
{
    std::unique_ptr<RenderStage> StageFactory::CreateComputeStage(
        const String& stageName,
        const String& shaderPath,
        const String& outputBufferName,
        uint32_t workgroupSizeX,
        uint32_t workgroupSizeY)
    {
        StageConfiguration config;
        config.name = stageName;
        config.type = PipelineType::COMPUTE;

        // Add compute shader
        config.shaders.push_back({
            .stage = ShaderStage::COMPUTE,
            .path = shaderPath
        });

        // Add output buffer binding
        config.outputBuffers.push_back({
            .bufferName = outputBufferName,
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .shaderStages = VK_SHADER_STAGE_COMPUTE_BIT
        });

        // Set compute configuration
        ComputeConfig computeConfig;
        computeConfig.workgroupSizeX = workgroupSizeX;
        computeConfig.workgroupSizeY = workgroupSizeY;
        computeConfig.workgroupSizeZ = 1;
        config.pipelineConfig = computeConfig;

        return std::make_unique<RenderStage>(config);
    }

    std::unique_ptr<RenderStage> StageFactory::CreateComputeStageAdvanced(
        const String& stageName,
        const String& shaderPath,
        const Vector<BufferBinding>& inputs,
        const Vector<BufferBinding>& outputs,
        const ComputeConfig& computeConfig)
    {
        StageConfiguration config;
        config.name = stageName;
        config.type = PipelineType::COMPUTE;

        // Add compute shader
        config.shaders.push_back({
            .stage = ShaderStage::COMPUTE,
            .path = shaderPath
        });

        config.inputBuffers = inputs;
        config.outputBuffers = outputs;
        config.pipelineConfig = computeConfig;

        return std::make_unique<RenderStage>(config);
    }

    std::unique_ptr<RenderStage> StageFactory::CreateGraphicsStage(
        const String& stageName,
        const String& vertexShaderPath,
        const String& fragmentShaderPath,
        const String& outputBufferName,
        VkFormat colorFormat)
    {
        StageConfiguration config;
        config.name = stageName;
        config.type = PipelineType::GRAPHICS;

        // Add vertex and fragment shaders
        config.shaders.push_back({
            .stage = ShaderStage::VERTEX,
            .path = vertexShaderPath
        });

        config.shaders.push_back({
            .stage = ShaderStage::FRAGMENT,
            .path = fragmentShaderPath
        });

        // Add output buffer binding
        config.outputBuffers.push_back({
            .bufferName = outputBufferName,
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT
        });

        // Set graphics configuration
        GraphicsConfig graphicsConfig;
        graphicsConfig.colorAttachmentFormat = colorFormat;
        config.pipelineConfig = graphicsConfig;

        return std::make_unique<RenderStage>(config);
    }

    std::unique_ptr<RenderStage> StageFactory::CreateFromConfiguration(
        const StageConfiguration& config)
    {
        return std::make_unique<RenderStage>(config);
    }
}

