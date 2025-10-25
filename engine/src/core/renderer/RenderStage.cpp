#include <magma_engine/core/renderer/RenderStage.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <logging/Logger.h>

namespace Magma
{
    RenderStage::RenderStage(const StageConfiguration& config)
        : m_config(config)
    {
        Logger::Log(LogLevel::DEBUG, "[RenderStage] Created stage: {}", m_config.name);
    }

    String RenderStage::GetStageName() const
    {
        return m_config.name;
    }

    Vector<BufferRequirement> RenderStage::GetBufferRequirements() const
    {
        return GenerateBufferRequirements();
    }

    void RenderStage::Initialize(
        VkDevice device,
        BufferRegistry& bufferRegistry,
        std::shared_ptr<DescriptorManager> descriptorManager)
    {
        if (m_initialized)
        {
            Logger::Log(LogLevel::WARNING, "[RenderStage:{}] Already initialized", m_config.name);
            return;
        }

        m_descriptorManager = descriptorManager;

        Logger::Log(LogLevel::INFO, "[RenderStage:{}] Initializing {} pipeline",
            m_config.name, m_config.IsCompute() ? "compute" : "graphics");

        // Get extent from first output buffer
        if (!m_config.outputBuffers.empty())
        {
            auto firstBuffer = bufferRegistry.GetBuffer(m_config.outputBuffers[0].bufferName);
            if (firstBuffer)
            {
                m_currentExtent = {firstBuffer->imageExtent.width, firstBuffer->imageExtent.height};
            }
        }

        // Load shaders
        LoadShaders(device);

        // Create descriptor resources
        CreateDescriptorLayouts();
        AllocateDescriptors();
        UpdateDescriptorSets(bufferRegistry);

        // Create pipeline
        CreatePipeline(device);

        m_initialized = true;
        Logger::Log(LogLevel::INFO, "[RenderStage:{}] Initialization complete", m_config.name);
    }

    void RenderStage::Execute(VkCommandBuffer cmd)
    {
        if (!m_initialized)
        {
            Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Not initialized", m_config.name);
            return;
        }

        if (m_config.IsCompute())
        {
            ExecuteCompute(cmd);
        }
        else
        {
            ExecuteGraphics(cmd);
        }
    }

    void RenderStage::Cleanup()
    {
        Logger::Log(LogLevel::INFO, "[RenderStage:{}] Cleaning up", m_config.name);

        // Destroy pipeline
        if (m_config.IsCompute())
        {
            std::get<ComputePipeline>(m_pipeline).Destroy();
        }
        else
        {
            std::get<GraphicsPipeline>(m_pipeline).Destroy();
        }

        // Destroy shaders
        for (auto& shader : m_shaderModules)
        {
            shader.Destroy();
        }
        m_shaderModules.clear();

        // Descriptor cleanup is handled by DescriptorManager

        m_initialized = false;
    }

    void RenderStage::OnResolutionChanged(VkExtent2D newExtent)
    {
        m_currentExtent = newExtent;
        Logger::Log(LogLevel::INFO, "[RenderStage:{}] Resolution changed to {}x{}",
            m_config.name, newExtent.width, newExtent.height);
    }

    StageDebugInfo RenderStage::GetDebugInfo() const
    {
        StageDebugInfo info;
        info.stageName = m_config.name;

        for (const auto& input : m_config.inputBuffers)
        {
            info.inputBuffers.push_back(input.bufferName);
        }

        for (const auto& output : m_config.outputBuffers)
        {
            info.outputBuffers.push_back(output.bufferName);
        }

        info.metadata["type"] = m_config.IsCompute() ? "compute" : "graphics";
        info.metadata["resolution"] = std::to_string(m_currentExtent.width) + "x" + std::to_string(m_currentExtent.height);

        return info;
    }

    void RenderStage::UpdateConfiguration(const StageConfiguration& config)
    {
        if (m_initialized)
        {
            Logger::Log(LogLevel::WARNING, "[RenderStage:{}] Cannot update configuration while initialized", m_config.name);
            return;
        }

        m_config = config;
        Logger::Log(LogLevel::INFO, "[RenderStage:{}] Configuration updated", m_config.name);
    }

    void RenderStage::LoadShaders(VkDevice device)
    {
        m_shaderModules.clear();
        m_shaderModules.reserve(m_config.shaders.size());

        for (const auto& shaderBinding : m_config.shaders)
        {
            ShaderModule shader;
            if (!shader.LoadFromFile(device, shaderBinding.path))
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Failed to load shader: {}",
                    m_config.name, shaderBinding.path);
                continue;
            }

            m_shaderModules.push_back(std::move(shader));
            Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Loaded shader: {}",
                m_config.name, shaderBinding.path);
        }
    }

    void RenderStage::CreateDescriptorLayouts()
    {
        Vector<DescriptorLayoutBinding> bindings;

        // Combine input and output buffer bindings
        for (const auto& input : m_config.inputBuffers)
        {
            bindings.push_back({
                .binding = input.binding,
                .type = input.descriptorType,
                .stageFlags = input.shaderStages
            });
        }

        for (const auto& output : m_config.outputBuffers)
        {
            bindings.push_back({
                .binding = output.binding,
                .type = output.descriptorType,
                .stageFlags = output.shaderStages
            });
        }

        if (!bindings.empty())
        {
            m_descriptorLayout = m_descriptorManager->CreateLayout(bindings);
            Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Created descriptor layout with {} bindings",
                m_config.name, bindings.size());
        }
    }

    void RenderStage::AllocateDescriptors()
    {
        if (m_descriptorLayout != VK_NULL_HANDLE)
        {
            m_descriptorSet = m_descriptorManager->AllocateDescriptorSet(m_descriptorLayout);
            Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Allocated descriptor set", m_config.name);
        }
    }

    void RenderStage::UpdateDescriptorSets(BufferRegistry& bufferRegistry)
    {
        if (m_descriptorSet == VK_NULL_HANDLE)
        {
            return;
        }

    	// TODO: Doesn't need to be two loops, can be combined.
    	// TODO: Handle other descriptor types (e.g., uniform buffers) as needed.
        // Update input buffers
        for (const auto& input : m_config.inputBuffers)
        {
            auto buffer = bufferRegistry.GetBuffer(input.bufferName);
            if (!buffer)
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Input buffer '{}' not found",
                    m_config.name, input.bufferName);
                continue;
            }

            m_descriptorManager->WriteImageDescriptor(
                m_descriptorSet,
                input.binding,
                buffer->imageView,
                VK_IMAGE_LAYOUT_GENERAL,
                input.descriptorType
            );
        }

        // Update output buffers
        for (const auto& output : m_config.outputBuffers)
        {
            auto buffer = bufferRegistry.GetBuffer(output.bufferName);
            if (!buffer)
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Output buffer '{}' not found",
                    m_config.name, output.bufferName);
                continue;
            }

            m_descriptorManager->WriteImageDescriptor(
                m_descriptorSet,
                output.binding,
                buffer->imageView,
                VK_IMAGE_LAYOUT_GENERAL,
                output.descriptorType
            );
        }

        Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Updated descriptor sets", m_config.name);
    }

    void RenderStage::CreatePipeline(VkDevice device)
    {
        PipelineLayoutInfo layoutInfo{};

        if (m_descriptorLayout != VK_NULL_HANDLE)
        {
            layoutInfo.descriptorSetLayouts.push_back(m_descriptorLayout);
        }

        if (m_config.pushConstants.has_value())
        {
            VkPushConstantRange range{};
            range.stageFlags = m_config.pushConstants->stageFlags;
            range.offset = m_config.pushConstants->offset;
            range.size = m_config.pushConstants->size;
            layoutInfo.pushConstantRanges.push_back(range);
        }

        if (m_config.IsCompute())
        {
            if (m_shaderModules.empty())
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] No compute shader loaded", m_config.name);
                return;
            }

            // Construct ComputePipeline directly in the variant
            m_pipeline.emplace<ComputePipeline>();
            auto& pipeline = std::get<ComputePipeline>(m_pipeline);

            if (!pipeline.Create(device, layoutInfo, m_shaderModules[0].GetModule()))
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Failed to create compute pipeline", m_config.name);
                return;
            }

            Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Created compute pipeline", m_config.name);
        }
        else
        {
            if (m_shaderModules.size() < 2)
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Graphics pipeline requires vertex and fragment shaders", m_config.name);
                return;
            }

            const auto& graphicsConfig = m_config.GetGraphicsConfig();

            // Construct GraphicsPipeline directly in the variant
            m_pipeline.emplace<GraphicsPipeline>();
            auto& pipeline = std::get<GraphicsPipeline>(m_pipeline);

            if (!pipeline.Create(
                device,
                layoutInfo,
                m_shaderModules[0].GetModule(),  // Vertex shader
                m_shaderModules[1].GetModule(),  // Fragment shader
                graphicsConfig.colorAttachmentFormat,
                graphicsConfig.depthAttachmentFormat))
            {
                Logger::Log(LogLevel::ERROR, "[RenderStage:{}] Failed to create graphics pipeline", m_config.name);
                return;
            }

            Logger::Log(LogLevel::DEBUG, "[RenderStage:{}] Created graphics pipeline", m_config.name);
        }
    }

    Vector<BufferRequirement> RenderStage::GenerateBufferRequirements() const
    {
        Vector<BufferRequirement> requirements;

        // Generate requirements from input buffers
        for (const auto& input : m_config.inputBuffers)
        {
            BufferRequirement req{};
            req.name = input.bufferName;
            req.format = VK_FORMAT_R16G16B16A16_SFLOAT;  // Default format, can be configured later
            req.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            req.matchSwapchainExtent = true;
            req.extent = {0, 0};
            req.expectedLayout = VK_IMAGE_LAYOUT_GENERAL;
            req.isInput = true;
            req.isOutput = false;

            requirements.push_back(req);
        }

        // Generate requirements from output buffers
        for (const auto& output : m_config.outputBuffers)
        {
            BufferRequirement req{};
            req.name = output.bufferName;
            req.format = VK_FORMAT_R16G16B16A16_SFLOAT;  // Default format, can be configured later
            req.usage = VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT;
            req.matchSwapchainExtent = true;
            req.extent = {0, 0};
            req.expectedLayout = VK_IMAGE_LAYOUT_GENERAL;
            req.isInput = false;
            req.isOutput = true;

            requirements.push_back(req);
        }

        return requirements;
    }

    void RenderStage::ExecuteCompute(VkCommandBuffer cmd)
    {
        auto& computePipeline = std::get<ComputePipeline>(m_pipeline);

        computePipeline.Bind(cmd);

        if (m_descriptorSet != VK_NULL_HANDLE)
        {
            std::vector<VkDescriptorSet> sets = { m_descriptorSet };
            m_descriptorManager->BindDescriptor(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline, sets);
        }

        // Calculate dispatch size based on workgroup configuration
        const auto& computeConfig = m_config.GetComputeConfig();
        uint32_t groupCountX = (m_currentExtent.width + computeConfig.workgroupSizeX - 1) / computeConfig.workgroupSizeX;
        uint32_t groupCountY = (m_currentExtent.height + computeConfig.workgroupSizeY - 1) / computeConfig.workgroupSizeY;
        uint32_t groupCountZ = computeConfig.workgroupSizeZ;

        computePipeline.Dispatch(cmd, groupCountX, groupCountY, groupCountZ);
    }

    void RenderStage::ExecuteGraphics(VkCommandBuffer cmd)
    {
        auto& graphicsPipeline = std::get<GraphicsPipeline>(m_pipeline);

        graphicsPipeline.Bind(cmd);

        if (m_descriptorSet != VK_NULL_HANDLE)
        {
            std::vector<VkDescriptorSet> sets = { m_descriptorSet };
            m_descriptorManager->BindDescriptor(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline, sets);
        }

        // TODO: Actual graphics commands (draw calls, etc.)
        // This will be expanded in later phases
        Logger::Log(LogLevel::WARNING, "[RenderStage:{}] Graphics pipeline execution not yet fully implemented", m_config.name);
    }
}
