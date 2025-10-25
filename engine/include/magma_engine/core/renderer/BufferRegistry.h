#pragma once

#include <memory>
#include <types/Containers.h>
#include <types/VkTypes.h>
#include <magma_engine/core/renderer/Image.h>

namespace Magma
{
    class BufferRegistry
    {
    public:
        BufferRegistry() = default;
        ~BufferRegistry() = default;

        void RegisterBuffer(const String& name, std::shared_ptr<AllocatedImage> buffer);
        std::shared_ptr<AllocatedImage> GetBuffer(const String& name) const;
        bool HasBuffer(const String& name) const;
        Vector<String> GetAllBufferNames() const;
        void Clear();

    private:
        Map<String, std::shared_ptr<AllocatedImage>> m_buffers;
    };
}
