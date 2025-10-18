#pragma once

#include "gui/panes/IPane.h"

namespace Magma
{
    class SceneHierarchyPane : public IPane
    {
    public:
        SceneHierarchyPane() = default;
        ~SceneHierarchyPane() override = default;

        void Render() override;
        const char* GetName() const override { return "Scene Hierarchy"; }
    };
}
