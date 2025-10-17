#pragma once
#include <vulkan/vulkan.h>

namespace Magma::vkutil
{
	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize, PFN_vkCmdBlitImage2KHR vkCmdBlitImage2Func);
}