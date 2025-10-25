#pragma once
#include <types/VkTypes.h>

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};
