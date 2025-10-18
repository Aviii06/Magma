#include "ViewportPane.h"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <magma_engine/core/renderer/Renderer.h>

namespace Magma
{
    ViewportPane::ViewportPane(std::shared_ptr<Renderer> renderer)
        : m_renderer(renderer)
    {
    }

    void ViewportPane::init_viewport_texture()
    {
        if (m_textureInitialized) return;

        // Create ImGui texture from the draw image
        auto& drawImage = m_renderer->GetDrawImage();

        m_viewportTextureID = ImGui_ImplVulkan_AddTexture(
            m_renderer->GetDrawImageSampler(),  // Use the proper sampler
            drawImage.imageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        m_textureInitialized = true;
    }

    void ViewportPane::Render()
    {
        if (!m_textureInitialized)
        {
            init_viewport_texture();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(GetName());

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

        if (m_viewportTextureID != VK_NULL_HANDLE)
        {
            ImGui::Image(
                m_viewportTextureID,
                viewportPanelSize,
                ImVec2(0, 0),
                ImVec2(1, 1)
            );
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
