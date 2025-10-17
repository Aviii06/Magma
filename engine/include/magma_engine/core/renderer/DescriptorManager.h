#pragma once
#include <types/VkTypes.h>
#include <types/Containers.h>

namespace Magma
{
    struct DescriptorLayoutBinding
    {
        uint32_t binding;
        VkDescriptorType type;
        VkShaderStageFlags stageFlags;
        uint32_t descriptorCount = 1;
    };

    class DescriptorManager
    {
    public:
        DescriptorManager() = default;
        ~DescriptorManager();

        // Delete copy operations
        DescriptorManager(const DescriptorManager&) = delete;
        DescriptorManager& operator=(const DescriptorManager&) = delete;

        void Init(VkDevice device);
        void Cleanup();

        // Create descriptor set layout from bindings
        VkDescriptorSetLayout CreateLayout(const Vector<DescriptorLayoutBinding>& bindings);

        // Destroy a specific layout
        void DestroyLayout(VkDescriptorSetLayout layout);

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        Vector<VkDescriptorSetLayout> m_layouts;
    };
}

