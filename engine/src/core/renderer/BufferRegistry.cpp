#include <magma_engine/core/renderer/BufferRegistry.h>

namespace Magma
{
    void BufferRegistry::RegisterBuffer(const String& name, std::shared_ptr<AllocatedImage> buffer)
    {
        m_buffers[name] = buffer;
    }

    std::shared_ptr<AllocatedImage> BufferRegistry::GetBuffer(const String& name) const
    {
        auto it = m_buffers.find(name);
        if (it != m_buffers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool BufferRegistry::HasBuffer(const String& name) const
    {
        return m_buffers.find(name) != m_buffers.end();
    }

    Vector<String> BufferRegistry::GetAllBufferNames() const
    {
        Vector<String> names;
        names.reserve(m_buffers.size());
        for (const auto& [name, _] : m_buffers)
        {
            names.push_back(name);
        }
        return names;
    }

    void BufferRegistry::Clear()
    {
        m_buffers.clear();
    }
}

