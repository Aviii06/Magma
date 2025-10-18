#include "PropertiesPane.h"
#include <imgui.h>

namespace Magma
{
    void PropertiesPane::Render()
    {
        ImGui::Begin(GetName());

        ImGui::Text("Properties Panel");
        ImGui::Separator();

        // Placeholder content - will be filled with actual properties later
        ImGui::Text("Select an object to view properties");

        ImGui::End();
    }
}

