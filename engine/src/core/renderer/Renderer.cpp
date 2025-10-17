#include <VkBootstrap.h>
#include <types/Containers.h>

#include <magma_engine/ServiceLocater.h>
#include <magma_engine/Window.h>
#include <magma_engine/core/renderer/VkInitializers.h>
#include <magma_engine/core/renderer/VkUtils.h>
#include <magma_engine/core/renderer/Renderer.h>

constexpr bool b_UseValidationLayers = true;

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

void Magma::Renderer::Init()
{
	m_frames.resize(FRAME_OVERLAP);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_sync_structures();
	init_pipelines();
}

void Magma::Renderer::Cleanup()
{
	vkDeviceWaitIdle(m_device);

	// Cleanup pipelines and shaders
	m_trianglePipeline.Destroy();
	m_triangleVertShader.Destroy();
	m_triangleFragShader.Destroy();
	m_descriptorManager.Cleanup();

	// Cleanup sync structures
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		vkDestroyFence(m_device, m_frames[i].m_renderFence, nullptr);
		vkDestroySemaphore(m_device, m_frames[i].m_swapchainSemaphore, nullptr);
		vkDestroySemaphore(m_device, m_frames[i].m_renderSemaphore, nullptr);
		vkDestroyCommandPool(m_device, m_frames[i].m_commandPool, nullptr);
	}

	destroy_swapchain();

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
	vkDestroyInstance(m_instance, nullptr);
}

void Magma::Renderer::Draw()
{
	VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().m_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().m_renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().m_swapchainSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = get_current_frame().m_mainCommandBuffer;

	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Transition image to color attachment for rendering
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	draw_geometry(cmd, swapchainImageIndex);

	// Transition image to present layout
	vkutil::transition_image(cmd, m_swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	auto inst_ret = builder.set_app_name("Magma Engine")
						   .use_default_debug_messenger()
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

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	auto physicalDeviceResult = selector
		.set_minimum_version(1, 2)
		.set_surface(m_surface)
		.add_required_extension("VK_KHR_dynamic_rendering")
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

	// For dynamic rendering
	m_vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderingKHR");
	m_vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(m_device, "vkCmdEndRenderingKHR");

	assert(m_vkCmdBeginRenderingKHR && "Dynamic Rendering not available!");
	assert(m_vkCmdEndRenderingKHR && "Dynamic Rendering not available!");
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
		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = m_frames[i].m_commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].m_mainCommandBuffer));
	}
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

void Magma::Renderer::init_pipelines()
{
	m_descriptorManager.Init(m_device);

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

	PipelineLayoutInfo layoutInfo{};

	if (!m_trianglePipeline.CreateGraphicsPipeline(
		m_device,
		layoutInfo,
		m_triangleVertShader.GetModule(),
		m_triangleFragShader.GetModule(),
		m_swapchainImageFormat))
	{
		Logger::Log(LogLevel::ERROR, "Failed to create triangle pipeline");
		std::exit(EXIT_FAILURE);
	}

	Logger::Log(LogLevel::INFO, "Pipelines initialized successfully");
}

void Magma::Renderer::init_background_pipelines()
{
}

void Magma::Renderer::draw_background(VkCommandBuffer cmd)
{
}

void Magma::Renderer::draw_geometry(VkCommandBuffer cmd, uint32_t swapchainImageIndex)
{
	VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(
		m_swapchainImageViews[swapchainImageIndex],
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
