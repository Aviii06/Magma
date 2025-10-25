#pragma once

#include <types/Containers.h>
#include <types/VkTypes.h>
#include <variant>
#include <optional>

namespace Magma
{
    enum class PipelineType
    {
        COMPUTE,
        GRAPHICS
    };

    enum class ShaderStage
    {
        VERTEX,
        FRAGMENT,
        COMPUTE
    };

    struct ShaderBinding
    {
        ShaderStage stage;
        String path;
    };

    struct BufferBinding
    {
        String bufferName;
        uint32_t binding;
        VkDescriptorType descriptorType;
        VkShaderStageFlags shaderStages;
    };

    struct ComputeConfig
    {
        uint32_t workgroupSizeX = 16;
        uint32_t workgroupSizeY = 16;
        uint32_t workgroupSizeZ = 1;
    };

    struct GraphicsConfig
    {
        VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;

        bool enableDepthTest = false;
        bool enableBlending = false;
    };

    struct PushConstantConfig
    {
        VkShaderStageFlags stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    struct StageConfiguration
    {
        String name;
        PipelineType type;

        // Shaders
        Vector<ShaderBinding> shaders;

        Vector<BufferBinding> inputBuffers;
        Vector<BufferBinding> outputBuffers;

        std::variant<ComputeConfig, GraphicsConfig> pipelineConfig;

        std::optional<PushConstantConfig> pushConstants;

        bool IsCompute() const { return type == PipelineType::COMPUTE; }
        bool IsGraphics() const { return type == PipelineType::GRAPHICS; }

        const ComputeConfig& GetComputeConfig() const
        {
            return std::get<ComputeConfig>(pipelineConfig);
        }

        const GraphicsConfig& GetGraphicsConfig() const
        {
            return std::get<GraphicsConfig>(pipelineConfig);
        }
    };
}
