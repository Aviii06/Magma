#include <magma_engine/Window.h>
#include <exceptions/MagmaRuntimeException.h>

void Magma::Window::OpenWindow(WindowData&& data)
{
	m_windowData = std::move(data);

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_window.reset(glfwCreateWindow(
		m_windowData.m_width,
		m_windowData.m_height,
		m_windowData.m_title.c_str(), nullptr, nullptr));

	m_extensions = glfwGetRequiredInstanceExtensions(&m_extensionCount);

	glfwSetWindowUserPointer(m_window.get(), this);
	glfwSetFramebufferSizeCallback(m_window.get(), nullptr);
}

void Magma::Window::Cleanup()
{
	glfwDestroyWindow(m_window.get());
	glfwTerminate();
}

void Magma::Window::Update()
{
	glfwPollEvents();
}

void Magma::Window::GetDrawSurface(Map<SurfaceArgs, int*> surfaceArgs)
{
	auto vkInstance = reinterpret_cast<VkInstance>(surfaceArgs[SurfaceArgs::INSTANCE]);
	auto *allocationCallbacks = surfaceArgs[SurfaceArgs::ALLOCATORS] ?
			reinterpret_cast<VkAllocationCallbacks *>(surfaceArgs[SurfaceArgs::ALLOCATORS]): nullptr;
	auto *outSurface = reinterpret_cast<VkSurfaceKHR*>(surfaceArgs[SurfaceArgs::OUT_SURFACE]);

	if (vkInstance == VK_NULL_HANDLE)
	{
		throw MagmaRuntimeException("Must provide an instance!");
	}

	if (glfwCreateWindowSurface(vkInstance, m_window.get(), allocationCallbacks, outSurface) != VK_SUCCESS)
	{
		throw MagmaRuntimeException("GLFW surface creation error");
	}
}
