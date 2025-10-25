#include <magma_engine/core/renderer/ShaderModule.h>
#include <fstream>
#include <iostream>

namespace Magma
{
    ShaderModule::ShaderModule(ShaderModule&& other) noexcept
        : m_device(other.m_device)
        , m_shaderModule(other.m_shaderModule)
    {
        other.m_device = VK_NULL_HANDLE;
        other.m_shaderModule = VK_NULL_HANDLE;
    }

    ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_device = other.m_device;
            m_shaderModule = other.m_shaderModule;
            other.m_device = VK_NULL_HANDLE;
            other.m_shaderModule = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool ShaderModule::LoadFromFile(VkDevice device, const std::string& filePath)
    {
        m_device = device;

        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << filePath << std::endl;
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        Vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode = buffer.data();

        if (vkCreateShaderModule(m_device, &createInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
        {
            std::cerr << "Failed to create shader module from: " << filePath << std::endl;
            return false;
        }

        return true;
    }

    void ShaderModule::Destroy()
    {
        if (m_shaderModule != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
            m_shaderModule = VK_NULL_HANDLE;
        }
    }
}

