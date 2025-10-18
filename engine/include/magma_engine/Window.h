#pragma once
#include <types/Containers.h>
#include <maths/Vec.h>
#include <magma_engine/ServiceLocater.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>



namespace Magma
{
	struct WindowData
	{
		String m_title;
		uint32_t m_width, m_height;
	};

	enum class SurfaceArgs
	{
		INSTANCE,
		ALLOCATORS,
		OUT_SURFACE
	};

	class Window : public IService
	{
	public:
		Window() : m_window(nullptr, glfwDestroyWindow) {}
		void Cleanup() override;
		void OpenWindow(WindowData&& data);
		void Update();
		bool ShouldClose() const { return glfwWindowShouldClose(m_window.get()); }
		void GetDrawSurface(Map<SurfaceArgs, int*> surfaceArgs);
		Maths::Vec2<uint32_t> GetExtent() { return {m_windowData.m_width, m_windowData.m_height}; }
		GLFWwindow* GetGLFWWindow() const { return m_window.get(); }
		const char** GetRequiredExtensions(uint32_t* count) const
		{
			*count = m_extensionCount;
			return m_extensions;
		}
		~Window() = default;

	private:
		WindowData m_windowData;
		std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;
		const char** m_extensions;
		uint32_t m_extensionCount = 0;
	};
}
