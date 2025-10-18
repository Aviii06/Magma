#pragma once

#include <memory>
#include <vector>
#include "gui/panes/IPane.h"

namespace Magma
{
    class GuiRenderer
    {
    public:
        GuiRenderer() = default;
        ~GuiRenderer() = default;

        void AddPane(std::unique_ptr<IPane> pane);
        void RenderAllPanes();

    private:
        void setup_dockspace();

    private:
        std::vector<std::unique_ptr<IPane>> m_panes;
    };
}
