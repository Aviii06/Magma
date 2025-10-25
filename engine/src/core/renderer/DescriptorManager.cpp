#include <magma_engine/core/renderer/DescriptorManager.h>
#include <logging/Logger.h>

namespace Magma
{
    DescriptorManager::~DescriptorManager()
    {
        Cleanup();
    }

    void DescriptorManager::Init(VkDevice device)
    {
        m_device = device;

        // Initialize the global descriptor allocator with common descriptor types
        // These ratios are based on typical usage patterns
        std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.0f },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1.0f },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1.0f },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f }
        };

        // Initialize with 10 max sets (can be adjusted based on needs)
        m_globalDescriptorAllocator.init_pool(device, 10, poolRatios);
    }

    void DescriptorManager::Cleanup()
    {
        // Cleanup descriptor set layouts first
        for (auto layout : m_layouts)
        {
            if (layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
            }
        }
        m_layouts.clear();

        // Then cleanup the global descriptor allocator (which destroys the pool and all descriptor sets)
		m_globalDescriptorAllocator.destroy_pool(m_device);

        m_device = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayout DescriptorManager::CreateLayout(const Vector<DescriptorLayoutBinding>& bindings)
    {
    	if (m_device == VK_NULL_HANDLE)
		{
			Logger::Log(LogLevel::ERROR, "DescriptorManager not initialized");
			return VK_NULL_HANDLE;
		}

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
            Logger::Log(LogLevel::ERROR, "Failed to create descriptor set layout");
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

    VkDescriptorSet DescriptorManager::AllocateDescriptorSet(VkDescriptorSetLayout layout)
    {
        if (m_device == VK_NULL_HANDLE)
        {
            Logger::Log(LogLevel::ERROR, "DescriptorManager not initialized");
            return VK_NULL_HANDLE;
        }

        return m_globalDescriptorAllocator.allocate(m_device, layout);
    }

	void DescriptorManager::BindDescriptor(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, const Pipeline& pipeline, std::vector<VkDescriptorSet>& sets)
    {
    	vkCmdBindDescriptorSets(cmd,
    		bindPoint,
    		pipeline.GetLayout(),
    		0,
    		static_cast<uint32_t>(sets.size()),
    		sets.data(),
    		0,
    		nullptr);
    }

    void DescriptorManager::WriteImageDescriptor(
        VkDescriptorSet set,
        uint32_t binding,
        VkImageView imageView,
        VkImageLayout layout,
        VkDescriptorType type)
    {
        if (m_device == VK_NULL_HANDLE)
        {
            Logger::Log(LogLevel::ERROR, "DescriptorManager not initialized");
            return;
        }

        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = layout;
        imgInfo.imageView = imageView;

        VkWriteDescriptorSet writeSet = {};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.pNext = nullptr;
        writeSet.dstBinding = binding;
        writeSet.dstSet = set;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = type;
        writeSet.pImageInfo = &imgInfo;

        vkUpdateDescriptorSets(m_device, 1, &writeSet, 0, nullptr);
    }

	void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
    {
    	std::vector<VkDescriptorPoolSize> poolSizes;
    	for (PoolSizeRatio ratio : poolRatios) {
    		poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.type,
				.descriptorCount = uint32_t(ratio.ratio * maxSets)
			});
    	}

    	VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    	pool_info.flags = 0;
    	pool_info.maxSets = maxSets;
    	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    	pool_info.pPoolSizes = poolSizes.data();

    	VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));
    }

	void DescriptorAllocator::clear_descriptors(VkDevice device)
    {
    	vkResetDescriptorPool(device, pool, 0);
    }

	void DescriptorAllocator::destroy_pool(VkDevice device)
    {
    	if (pool != VK_NULL_HANDLE)
    	{
    		vkDestroyDescriptorPool(device, pool, nullptr);
    		pool = VK_NULL_HANDLE;
    	}
    }

	VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
    {
    	VkDescriptorSetAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    	allocInfo.pNext = nullptr;
    	allocInfo.descriptorPool = pool;
    	allocInfo.descriptorSetCount = 1;
    	allocInfo.pSetLayouts = &layout;

    	VkDescriptorSet ds;
    	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

    	return ds;
    }
}
