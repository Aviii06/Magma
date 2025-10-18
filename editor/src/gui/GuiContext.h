#pragma once

#include <memory>
#include <types/VkTypes.h>

namespace Magma
{
    class Window;
    class Renderer;

    class GuiContext
    {
    public:
        GuiContext(std::shared_ptr<Window> window, std::shared_ptr<Renderer> renderer);
        ~GuiContext();

        void BeginFrame();
        void EndFrame();
        void RenderToCommandBuffer(VkCommandBuffer cmd);

        void Cleanup();

    private:
        void init_imgui();
        void shutdown_imgui();

    private:
        std::shared_ptr<Window> m_window;
        std::shared_ptr<Renderer> m_renderer;
        VkDescriptorPool m_imguiDescriptorPool;
        bool m_initialized = false;
    };
}
