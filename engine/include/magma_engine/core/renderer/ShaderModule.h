#pragma once
#include <types/VkTypes.h>
#include <types/Containers.h>
#include <string>

namespace Magma
{
    class ShaderModule
    {
    public:
        ShaderModule() = default;
        ~ShaderModule();

        // Delete copy operations
        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;

        // Move operations
        ShaderModule(ShaderModule&& other) noexcept;
        ShaderModule& operator=(ShaderModule&& other) noexcept;

        bool LoadFromFile(VkDevice device, const std::string& filePath);
        void Destroy();

        VkShaderModule GetModule() const { return m_shaderModule; }
        bool IsValid() const { return m_shaderModule != VK_NULL_HANDLE; }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    };
}

