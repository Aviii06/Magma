#pragma once

#include "gui/panes/IPane.h"
#include <memory>
#include <types/VkTypes.h>

namespace Magma
{
    class Renderer;

    class ViewportPane : public IPane
    {
    public:
        explicit ViewportPane(std::shared_ptr<Renderer> renderer);
        ~ViewportPane() override = default;

        void Render() override;
        const char* GetName() const override { return "Viewport"; }

    private:
        void init_viewport_texture();

    private:
        std::shared_ptr<Renderer> m_renderer;
        VkDescriptorSet m_viewportTextureID = VK_NULL_HANDLE;
        bool m_textureInitialized = false;
    };
}
