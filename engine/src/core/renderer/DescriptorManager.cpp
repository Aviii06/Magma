#include <magma_engine/core/renderer/DescriptorManager.h>
#include <iostream>

namespace Magma
{
    DescriptorManager::~DescriptorManager()
    {
        Cleanup();
    }

    void DescriptorManager::Init(VkDevice device)
    {
        m_device = device;
    }

    void DescriptorManager::Cleanup()
    {
        for (auto layout : m_layouts)
        {
            if (layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
            }
        }
        m_layouts.clear();
        m_device = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayout DescriptorManager::CreateLayout(const Vector<DescriptorLayoutBinding>& bindings)
    {
        Vector<VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(bindings.size());

        for (const auto& binding : bindings)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = binding.binding;
            layoutBinding.descriptorType = binding.type;
            layoutBinding.descriptorCount = binding.descriptorCount;
            layoutBinding.stageFlags = binding.stageFlags;
            layoutBinding.pImmutableSamplers = nullptr;

            vkBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr;
        layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        layoutInfo.pBindings = vkBindings.data();
        layoutInfo.flags = 0;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
        {
            std::cerr << "Failed to create descriptor set layout" << std::endl;
            return VK_NULL_HANDLE;
        }

        m_layouts.push_back(layout);
        return layout;
    }

    void DescriptorManager::DestroyLayout(VkDescriptorSetLayout layout)
    {
        auto it = std::find(m_layouts.begin(), m_layouts.end(), layout);
        if (it != m_layouts.end())
        {
            vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
            m_layouts.erase(it);
        }
    }
}

