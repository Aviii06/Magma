#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <VkBootstrap.h>
#include <types/Containers.h>

#include <magma_engine/ServiceLocater.h>
#include <magma_engine/Window.h>
#include <magma_engine/core/renderer/VkInitializers.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <magma_engine/core/renderer/Renderer.h>

constexpr bool b_UseValidationLayers = true;


void Magma::Renderer::Init()
{
	m_frames.resize(FRAME_OVERLAP);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_sync_structures();
	init_descriptors();
	init_pipelines();
}

void Magma::Renderer::Cleanup()
{
	vkDeviceWaitIdle(m_device);

	// TODO: Add these to the main deletion queue.
	m_trianglePipeline.Destroy();
	m_gradientPipeline.Destroy();
	m_triangleVertShader.Destroy();
	m_triangleFragShader.Destroy();
	m_gradientCompShader.Destroy();

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

void Magma::Renderer::Draw()
{
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().m_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().m_renderFence));

	get_current_frame().m_deletionQueue.flush();

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().m_swapchainSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;

	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_drawExtent.width = m_drawImage.imageExtent.width;
	m_drawExtent.height = m_drawImage.imageExtent.height;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);
	// draw_geometry(cmd);

	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vkutil::copy_image_to_image(cmd, m_drawImage.image, m_swapchainImages[swapchainImageIndex], m_drawExtent, m_swapchainExtent, m_vkCmdBlitImage2);

	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

	// Use Vulkan 1.0 submit instead of VkSubmitInfo2
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

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame().m_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	m_frameNumber++;
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
	m_descriptorManager.Init(m_device);

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

	Vector<DescriptorLayoutBinding> drawImageBindings = {
		{
			.binding = 0,
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
		}
	};

	m_drawImageDescriptorLayout = m_descriptorManager.CreateLayout(drawImageBindings);
	m_drawImageDescriptors = m_descriptorManager.AllocateDescriptorSet(m_drawImageDescriptorLayout);

	// Write the draw image to the descriptor set
	m_descriptorManager.WriteImageDescriptor(
		m_drawImageDescriptors,
		0,  // binding
		m_drawImage.imageView,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
	);

	m_mainDeletionQueue.push_function([this]() {
		m_descriptorManager.Cleanup();
	});

	Logger::Log(LogLevel::INFO, "Draw image descriptors initialized successfully");
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

	VkExtent3D drawImageExtent = {
		size.x,
		size.y,
		1
	};

	//hardcoding the draw format to 32 bit float
	m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	m_drawImage.imageExtent = drawImageExtent;

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;  // Required for ImGui texture sampling

	VkImageCreateInfo rimg_info = vkinit::image_create_info(m_drawImage.imageFormat, drawImageUsages, drawImageExtent);

	//for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimg_allocinfo = {};
	rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(m_allocator, &rimg_info, &rimg_allocinfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(m_drawImage.imageFormat, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

	VK_CHECK(vkCreateImageView(m_device, &rview_info, nullptr, &m_drawImage.imageView));

	//add to deletion queues
	m_mainDeletionQueue.push_function([this]() {
		vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
		vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);
	});
}

void Magma::Renderer::destroy_swapchain()
{
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	for (int i = 0; i < m_swapchainImageViews.size(); i++)
	{
		vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
	}
}

void Magma::Renderer::init_pipelines()
{
	if (!m_triangleVertShader.LoadFromFile(m_device, "assets/shaders/colored_triangle.vert.spv"))
	{
		Logger::Log(LogLevel::ERROR, "Failed to load triangle vertex shader");
		std::exit(EXIT_FAILURE);
	}

	if (!m_triangleFragShader.LoadFromFile(m_device, "assets/shaders/colored_triangle.frag.spv"))
	{
		Logger::Log(LogLevel::ERROR, "Failed to load triangle fragment shader");
		std::exit(EXIT_FAILURE);
	}

	if (!m_gradientCompShader.LoadFromFile(m_device, "assets/shaders/Gradient.comp.spv"))
	{
		Logger::Log(LogLevel::ERROR, "Failed to load triangle compute shader");
		std::exit(EXIT_FAILURE);
	}

	PipelineLayoutInfo layoutInfo{};
	layoutInfo.descriptorSetLayouts.push_back(m_drawImageDescriptorLayout);

	if (!m_trianglePipeline.Create(
		m_device,
		layoutInfo,
		m_triangleVertShader.GetModule(),
		m_triangleFragShader.GetModule(),
		m_swapchainImageFormat))
	{
		Logger::Log(LogLevel::ERROR, "Failed to create triangle pipeline");
		std::exit(EXIT_FAILURE);
	}

	if (!m_gradientPipeline.Create(
		m_device,
		layoutInfo,
		m_gradientCompShader.GetModule()))
	{
		Logger::Log(LogLevel::ERROR, "Failed to create triangle compute pipeline");
		std::exit(EXIT_FAILURE);
	}

	m_mainDeletionQueue.push_function([this]() {
		m_gradientPipeline.Destroy();
	});

	Logger::Log(LogLevel::INFO, "Pipelines initialized successfully");
}

void Magma::Renderer::init_background_pipelines()
{
}

void Magma::Renderer::draw_background(VkCommandBuffer cmd)
{
	//make a clear-color from frame number. This will flash with a 120 frame period.
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(m_frameNumber / 120.f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	// vkCmdClearColorImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	m_gradientPipeline.Bind(cmd);

	std::vector<VkDescriptorSet> sets;
	sets.push_back(m_drawImageDescriptors);
	m_descriptorManager.BindDescriptor(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipeline, sets);
	m_gradientPipeline.Dispatch(cmd, std::ceil(m_drawExtent.width / 16.0), std::ceil(m_drawExtent.height / 16.0), 1);
}

void Magma::Renderer::draw_geometry(VkCommandBuffer cmd)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
		m_drawImage.imageView,
		nullptr,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	VkRenderingInfo renderInfo = vkinit::rendering_info(m_swapchainExtent, &colorAttachment, nullptr);

	// Use manually loaded function pointer through the vulkan extensions.
	if (m_vkCmdBeginRenderingKHR)
	{
		m_vkCmdBeginRenderingKHR(cmd, &renderInfo);
	}
	else
	{
		Logger::Log(LogLevel::ERROR, "Dynamic rendering not available!");
		return;
	}

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapchainExtent.width);
	viewport.height = static_cast<float>(m_swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchainExtent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	m_trianglePipeline.Bind(cmd);
	vkCmdDraw(cmd, 3, 1, 0, 0);

	m_vkCmdEndRenderingKHR(cmd);
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

	m_drawExtent.width = m_drawImage.imageExtent.width;
	m_drawExtent.height = m_drawImage.imageExtent.height;

	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	draw_background(cmd);
	// draw_geometry(cmd);

	// Transition draw image to shader read for ImGui viewport
	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Magma::Renderer::CopyToSwapchain()
{
	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;

	// Transition draw image for transfer
	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transition_image(cmd, m_swapchainImages[m_currentSwapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vkutil::copy_image_to_image(cmd, m_drawImage.image, m_swapchainImages[m_currentSwapchainImageIndex], m_drawExtent, m_swapchainExtent, m_vkCmdBlitImage2);

	// Transition draw image back to shader read for ImGui to sample it
	vkutil::transition_image(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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
