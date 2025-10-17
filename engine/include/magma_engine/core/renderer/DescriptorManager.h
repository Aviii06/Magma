#pragma once
#include <span>

#include <types/VkTypes.h>
#include <types/Containers.h>

#include "Pipeline.h"

namespace Magma
{
    struct DescriptorLayoutBinding
    {
        uint32_t binding;
        VkDescriptorType type;
        VkShaderStageFlags stageFlags;
        uint32_t descriptorCount = 1;
    };

    struct DescriptorAllocator
    {
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        VkDescriptorPool pool = VK_NULL_HANDLE;

        void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
        void clear_descriptors(VkDevice device);
        void destroy_pool(VkDevice device);

        VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
    };

    class DescriptorManager
    {
    public:
        DescriptorManager() = default;
        ~DescriptorManager();

        DescriptorManager(const DescriptorManager&) = delete;
        DescriptorManager& operator=(const DescriptorManager&) = delete;

        void Init(VkDevice device);
        void Cleanup();

        VkDescriptorSetLayout CreateLayout(const Vector<DescriptorLayoutBinding>& bindings);
        void DestroyLayout(VkDescriptorSetLayout layout);

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);

        DescriptorAllocator& GetGlobalAllocator() { return m_globalDescriptorAllocator; }

        void BindDescriptor(VkCommandBuffer cmd,
            VkPipelineBindPoint bindPoint,
            const Pipeline& pipeline,
            std::vector<VkDescriptorSet>& sets);

        void WriteImageDescriptor(
            VkDescriptorSet set,
            uint32_t binding,
            VkImageView imageView,
            VkImageLayout layout,
            VkDescriptorType type);


    private:
        VkDevice m_device = VK_NULL_HANDLE;
        Vector<VkDescriptorSetLayout> m_layouts;

        DescriptorAllocator m_globalDescriptorAllocator;
    };
}
