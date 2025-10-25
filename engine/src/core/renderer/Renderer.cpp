#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <VkBootstrap.h>
#include <types/Containers.h>

#include <magma_engine/ServiceLocater.h>
#include <magma_engine/Window.h>
#include <magma_engine/core/renderer/VkInitializers.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <magma_engine/core/renderer/Renderer.h>
#include <magma_engine/core/renderer/StageFactory.h>

constexpr bool b_UseValidationLayers = true;


void Magma::Renderer::Init()
{
	m_frames.resize(FRAME_OVERLAP);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_sync_structures();
	init_descriptors();
	init_render_stages();
}

void Magma::Renderer::Cleanup()
{
	vkDeviceWaitIdle(m_device);

	// Cleanup sync structures and per-frame resources
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		// TODO: Add these to the frames deletion queue.
		vkDestroyFence(m_device, m_frames[i].m_renderFence, nullptr);
		vkDestroySemaphore(m_device, m_frames[i].m_swapchainSemaphore, nullptr);
		vkDestroySemaphore(m_device, m_frames[i].m_renderSemaphore, nullptr);

		m_frames[i].m_deletionQueue.flush();
	}

	destroy_swapchain();

	m_mainDeletionQueue.flush();

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
	vkDestroyInstance(m_instance, nullptr);
}

void Magma::Renderer::init_vulkan()
{
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = ServiceLocator::Get<Window>()->GetRequiredExtensions(&glfwExtensionCount);

	vkb::InstanceBuilder builder = vkb::InstanceBuilder();

	// Custom debug callback that uses your Logger
	auto debug_callback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                         VkDebugUtilsMessageTypeFlagsEXT messageType,
	                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                         void* pUserData) -> VkBool32 {

		LogLevel level;
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			level = LogLevel::ERROR;
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			level = LogLevel::WARNING;
		} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			level = LogLevel::INFO;
		} else {
			level = LogLevel::DEBUG;
		}

		Logger::Log(level, "[Vulkan Validation] {}", pCallbackData->pMessage);
		return VK_FALSE;
	};

	auto inst_ret = builder.set_app_name("Magma Engine")
						   .request_validation_layers(b_UseValidationLayers)
						   .set_debug_callback(debug_callback)
						   .require_api_version(1, 3, 0);

	for (uint32_t i = 0; i < glfwExtensionCount; i++)
	{
		inst_ret.enable_extension(glfwExtensions[i]);
	}

	auto instance_result = inst_ret.build();

	if (!instance_result)
	{
		Logger::Log(LogLevel::ERROR, "Failed to create Vulkan instance: {}", instance_result.error().message());
		std::exit(EXIT_FAILURE);
	}

	vkb::Instance vkb_inst = instance_result.value();

	m_instance = vkb_inst.instance;
	m_debugMessenger = vkb_inst.debug_messenger;

	Map<SurfaceArgs, int*> surfaceArgs {
	                    {SurfaceArgs::INSTANCE, (int*)m_instance},
						{SurfaceArgs ::OUT_SURFACE, (int*)&m_surface}
	};

	ServiceLocator::Get<Window>()->GetDrawSurface(surfaceArgs);

	// Enable Vulkan 1.2 features including bufferDeviceAddress
	VkPhysicalDeviceVulkan12Features vulkan12Features{};
	vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12Features.bufferDeviceAddress = VK_TRUE;
	vulkan12Features.descriptorIndexing = VK_TRUE;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	auto physicalDeviceResult = selector
		.set_minimum_version(1, 2)
		.set_surface(m_surface)
		.set_required_features_12(vulkan12Features)
		.add_required_extension("VK_KHR_dynamic_rendering")
		.add_required_extension("VK_KHR_copy_commands2")
		.select();

	if (!physicalDeviceResult)
	{
		Logger::Log(LogLevel::ERROR, "Failed to select physical device: ", physicalDeviceResult.error().message());
		std::exit(EXIT_FAILURE);
	}

	vkb::PhysicalDevice physicalDevice = physicalDeviceResult.value();

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature{};
	dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeature.dynamicRendering = VK_TRUE;

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	deviceBuilder.add_pNext(&dynamicRenderingFeature);

	auto vkbDeviceResult = deviceBuilder.build();

	if (!vkbDeviceResult)
	{
		Logger::Log(LogLevel::ERROR, "Failed to create device: ", vkbDeviceResult.error().message());
		std::exit(EXIT_FAILURE);
	}

	vkb::Device vkbDevice = vkbDeviceResult.value();

	m_device = vkbDevice.device;
	m_physicalDevice = physicalDevice.physical_device;

	m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	// For dynamic rendering and copy commands
	m_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderingKHR");
	m_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR");
	m_vkCmdBlitImage2 = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(m_device, "vkCmdBlitImage2KHR");

	assert(m_vkCmdBeginRenderingKHR && "Dynamic Rendering not available!");
	assert(m_vkCmdEndRenderingKHR && "Dynamic Rendering not available!");
	assert(m_vkCmdBlitImage2 && "vkCmdBlitImage2KHR not available!");

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocatorInfo, &m_allocator);

	m_mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(m_allocator);
	});
}

void Magma::Renderer::init_swapchain()
{
	create_swapchain(ServiceLocator::Get<Window>()->GetExtent());
}

void Magma::Renderer::init_commands()
{
	VkCommandPoolCreateInfo commandPoolInfo =  {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = m_graphicsQueueFamily;

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{

		VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].m_commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_frames[i].m_commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].m_mainCommandBuffer));

		m_mainDeletionQueue.push_function([=]()
		{
			vkDestroyCommandPool(m_device, m_frames[i].m_commandPool, nullptr);
		});
	}

	// Creating immediate sync and render objects
	VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_immRenderData.immCommandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_immRenderData.immCommandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immRenderData.immCommandBuffer));

	m_mainDeletionQueue.push_function([this]
	{
		vkDestroyCommandPool(m_device, m_immRenderData.immCommandPool, nullptr);
	});
}

void Magma::Renderer::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].m_renderFence));

		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].m_renderSemaphore));
	}

	VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immRenderData.immFence));
	m_mainDeletionQueue.push_function([this]()
	{
		vkDestroyFence(m_device, m_immRenderData.immFence, nullptr);
	});
}

void Magma::Renderer::init_descriptors()
{
	// Create a sampler for the draw image (for ImGui texture display)
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_drawImageSampler));

	m_mainDeletionQueue.push_function([this]() {
		vkDestroySampler(m_device, m_drawImageSampler, nullptr);
	});

	Logger::Log(LogLevel::INFO, "Descriptor manager initialized successfully");
}

void Magma::Renderer::init_render_stages()
{
	Logger::Log(LogLevel::INFO, "Initializing render stages");

	// Create and initialize resource allocator
	m_resourceAllocator = std::make_shared<RenderResourceAllocator>();
	m_resourceAllocator->Initialize(m_device, m_allocator);

	// Add render stages to orchestrator
	m_renderOrchestrator.AddStage(StageFactory::CreateComputeStage(
		"BackgroundStage",
		"assets/shaders/Gradient.comp.spv",
		"drawImage",
		16,
		16
	));

	// Initialize orchestrator (will allocate buffers and initialize all stages)
	m_renderOrchestrator.Initialize(
		m_resourceAllocator,
		m_swapchainExtent
	);

	// Update draw extent from the allocated draw image
	auto drawImage = m_renderOrchestrator.GetBuffer("drawImage");
	if (drawImage)
	{
		m_drawExtent.width = drawImage->imageExtent.width;
		m_drawExtent.height = drawImage->imageExtent.height;
	}

	m_mainDeletionQueue.push_function([this]()
	{
		m_renderOrchestrator.Cleanup();
		if (m_resourceAllocator)
		{
			m_resourceAllocator->Cleanup();
		}
	});

	Logger::Log(LogLevel::INFO, "Render stages initialized successfully");
}

void Magma::Renderer::create_swapchain(Maths::Vec2<uint32_t> size)
{
	vkb::SwapchainBuilder swapchainBuilder{ m_physicalDevice, m_device, m_surface };

	m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.set_desired_format(VkSurfaceFormatKHR{ .format = m_swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(size.x, size.y)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	m_swapchainExtent = vkbSwapchain.extent;
	m_swapchain = vkbSwapchain.swapchain;
	m_swapchainImages = vkbSwapchain.get_images().value();
	m_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void Magma::Renderer::destroy_swapchain()
{
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	for (int i = 0; i < m_swapchainImageViews.size(); i++)
	{
		vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
	}
}

void Magma::Renderer::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VK_CHECK(vkResetFences(m_device, 1, &m_immRenderData.immFence));
	VK_CHECK(vkResetCommandBuffer(m_immRenderData.immCommandBuffer, 0));

	VkCommandBuffer cmd = m_immRenderData.immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);
	VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

	// submit command buffer to the queue and execute it.
	//  _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immRenderData.immFence));

	VK_CHECK(vkWaitForFences(m_device, 1, &m_immRenderData.immFence, true, 9999999999));
}

void Magma::Renderer::BeginFrame()
{
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().m_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().m_renderFence));

	get_current_frame().m_deletionQueue.flush();

	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().m_swapchainSemaphore, nullptr, &m_currentSwapchainImageIndex));

	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
}

void Magma::Renderer::RenderScene()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;

	std::shared_ptr<AllocatedImage> drawImage = m_renderOrchestrator.GetBuffer("drawImage");
	if (drawImage)
	{
		m_drawExtent.width = drawImage->imageExtent.width;
		m_drawExtent.height = drawImage->imageExtent.height;

		// Transition to GENERAL for compute shader
		vkutil::transition_image(cmd, drawImage->image, drawImage->currentLayout, VK_IMAGE_LAYOUT_GENERAL);
		drawImage->currentLayout = VK_IMAGE_LAYOUT_GENERAL;
	}

	// Execute render stages through orchestrator
	m_renderOrchestrator.Execute(cmd);

	// Transition draw image to shader read for ImGui viewport
	if (drawImage)
	{
		vkutil::transition_image(cmd, drawImage->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		drawImage->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
}

void Magma::Renderer::CopyToSwapchain()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;

	auto drawImage = m_renderOrchestrator.GetBuffer("drawImage");
	if (!drawImage)
	{
		return;
	}

	// Transition draw image for transfer
	vkutil::transition_image(cmd, drawImage->image, drawImage->currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	drawImage->currentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkutil::transition_image(cmd, m_swapchainImages[m_currentSwapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vkutil::copy_image_to_image(cmd, drawImage->image, m_swapchainImages[m_currentSwapchainImageIndex], m_drawExtent, m_swapchainExtent, m_vkCmdBlitImage2);

	// Transition draw image back to shader read for ImGui to sample it
	vkutil::transition_image(cmd, drawImage->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	drawImage->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Transition to color attachment for ImGui rendering
	vkutil::transition_image(cmd, m_swapchainImages[m_currentSwapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void Magma::Renderer::BeginUIRenderPass()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;
	// Begin rendering to swapchain image for UI
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
		m_swapchainImageViews[m_currentSwapchainImageIndex],
		nullptr,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // Load existing content
	VkRenderingInfo renderInfo = vkinit::rendering_info(m_swapchainExtent, &colorAttachment, nullptr);

	if (m_vkCmdBeginRenderingKHR)
	{
		m_vkCmdBeginRenderingKHR(cmd, &renderInfo);
	}
}

void Magma::Renderer::EndFrame()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;
	if (m_vkCmdEndRenderingKHR)
	{
		m_vkCmdEndRenderingKHR(cmd);
	}

	// Transition to present
	vkutil::transition_image(cmd, m_swapchainImages[m_currentSwapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));
}

void Magma::Renderer::Present()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;
	// Submit
	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame().m_swapchainSemaphore;
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame().m_renderSemaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, get_current_frame().m_renderFence));

	// Present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame().m_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &m_currentSwapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

	m_frameNumber++;
}
