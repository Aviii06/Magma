#pragma once

#include "gui/panes/IPane.h"

namespace Magma
{
    class PropertiesPane : public IPane
    {
    public:
        PropertiesPane() = default;
        ~PropertiesPane() override = default;

        void Render() override;
        const char* GetName() const override { return "Properties"; }
    };
}
