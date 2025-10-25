#pragma once

#include <magma_engine/core/renderer/StageConfiguration.h>
#include <magma_engine/core/renderer/ComputePipeline.h>
#include <magma_engine/core/renderer/GraphicsPipeline.h>
#include <magma_engine/core/renderer/ShaderModule.h>
#include <magma_engine/core/renderer/DescriptorManager.h>
#include <magma_engine/core/renderer/BufferRegistry.h>
#include <variant>
#include <memory>

namespace Magma
{
    struct BufferRequirement
    {
        String name;
        VkFormat format;
        VkImageUsageFlags usage;
        bool matchSwapchainExtent;
        VkExtent2D extent;
        VkImageLayout expectedLayout;
        bool isInput;
        bool isOutput;
    };

    struct StageDebugInfo
    {
        String stageName;
        Vector<String> inputBuffers;
        Vector<String> outputBuffers;
        Map<String, String> metadata;
    };

    class RenderStage
    {
    public:
        explicit RenderStage(const StageConfiguration& config);
        ~RenderStage() = default;

        // IRenderStage interface implementation
        String GetStageName() const;
        Vector<BufferRequirement> GetBufferRequirements() const;

        void Initialize(
            VkDevice device,
            BufferRegistry& bufferRegistry,
            std::shared_ptr<DescriptorManager> descriptorManager
        );

        void Execute(VkCommandBuffer cmd);

        void Cleanup();
        void OnResolutionChanged(VkExtent2D newExtent);
        StageDebugInfo GetDebugInfo() const;

        // Configuration access
        const StageConfiguration& GetConfiguration() const { return m_config; }
        void UpdateConfiguration(const StageConfiguration& config);

    private:
        void LoadShaders(VkDevice device);
        void CreateDescriptorLayouts();
        void CreatePipeline(VkDevice device);
        void AllocateDescriptors();
        void UpdateDescriptorSets(BufferRegistry& bufferRegistry);

        Vector<BufferRequirement> GenerateBufferRequirements() const;

        void ExecuteCompute(VkCommandBuffer cmd);
        void ExecuteGraphics(VkCommandBuffer cmd);

    private:
        StageConfiguration m_config;

        std::shared_ptr<DescriptorManager> m_descriptorManager = nullptr;
        VkExtent2D m_currentExtent = {0, 0};

        Vector<ShaderModule> m_shaderModules;

        std::variant<ComputePipeline, GraphicsPipeline> m_pipeline;

        VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

        bool m_initialized = false;
    };
}

