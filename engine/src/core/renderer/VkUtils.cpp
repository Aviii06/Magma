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

void Magma::vkutil::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize, PFN_vkCmdBlitImage2KHR vkCmdBlitImage2Func)
{
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

	blitRegion.srcOffsets[0] = { 0, 0, 0 };
	blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcSize.width);
	blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcSize.height);
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[0] = { 0, 0, 0 };
	blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstSize.width);
	blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstSize.height);
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2Func(cmd, &blitInfo);
}
