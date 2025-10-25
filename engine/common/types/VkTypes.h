#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <logging/Logger.h>


#define VK_CHECK(x)                                                 \
do                                                              \
{                                                               \
VkResult err = x;                                           \
if (err)                                                    \
{                                                           \
Logger::Log(LogLevel::ERROR, "Detected Vulkan error: {}", static_cast<int>(err)); \
abort();                                                \
}                                                           \
} while (0)