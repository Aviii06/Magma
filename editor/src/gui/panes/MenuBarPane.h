#pragma once

#include "gui/panes/IPane.h"

namespace Magma
{
    class MenuBarPane : public IPane
    {
    public:
        MenuBarPane() = default;
        ~MenuBarPane() override = default;

        void Render() override;
        const char* GetName() const override { return "MenuBar"; }
    };
}
