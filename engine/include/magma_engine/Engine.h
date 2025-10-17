#pragma once
#include <types/VkTypes.h>

namespace Magma
{
    class Engine
    {
    public:
        void Init();
        void Cleanup();
        void Run();

    private:
        uint32_t m_frameNumber {0};
    };
}