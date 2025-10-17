#pragma once

#include <types/Containers.h>
#include <logging/Logging.h>

namespace Magma
{
    class MagmaRuntimeException
    {
    public:
        MagmaRuntimeException(String msg)
        {
           Logger::Log(LogLevel::ERROR, msg);
        }

        String& what() noexcept
        {
            return m_message;
        }

    private:
        String m_message;

    };
}

