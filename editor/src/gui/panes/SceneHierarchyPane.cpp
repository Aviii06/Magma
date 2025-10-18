#include "SceneHierarchyPane.h"
#include <imgui.h>

namespace Magma
{
    void SceneHierarchyPane::Render()
    {
        ImGui::Begin(GetName());

        ImGui::Text("Scene Hierarchy");
        ImGui::Separator();

        // Placeholder content - will be filled with actual scene graph later
        if (ImGui::TreeNode("Scene Root"))
        {
            ImGui::Text("No objects in scene");
            ImGui::TreePop();
        }

        ImGui::End();
    }
}

