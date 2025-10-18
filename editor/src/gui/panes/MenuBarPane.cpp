#include "MenuBarPane.h"
#include <imgui.h>

namespace Magma
{
    void MenuBarPane::Render()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {}
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Viewport", nullptr, &m_visible);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About")) {}
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }
}
