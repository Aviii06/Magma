#pragma once
#include <vulkan/vulkan.h>

namespace Magma::vkutil
{
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}