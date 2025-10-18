#pragma once

namespace Magma
{
    class IPane
    {
    public:
        virtual ~IPane() = default;

        virtual void Render() = 0;
        virtual const char* GetName() const = 0;
        virtual bool IsVisible() const { return m_visible; }
        virtual void SetVisible(bool visible) { m_visible = visible; }

    protected:
        bool m_visible = true;
    };
}

