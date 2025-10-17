#include <magma_engine/core/renderer/VkUtils.h>

#include <magma_engine/core/renderer/VkInitializers.h>

void Magma::vkutil::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	imageBarrier.pNext = nullptr;

	imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;

	VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
	imageBarrier.image = image;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // srcStageMask
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // dstStageMask
		0,                                    // dependencyFlags
		0, nullptr,                          // memory barriers
		0, nullptr,                          // buffer barriers
		1, &imageBarrier);                   // image barriers
}
