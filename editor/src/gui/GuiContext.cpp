#include "GuiContext.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include <magma_engine/Window.h>
#include <magma_engine/core/renderer/Renderer.h>
#include <magma_engine/core/renderer/VkInitializers.h>
#include <logging/Logger.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Magma
{
    GuiContext::GuiContext(std::shared_ptr<Window> window, std::shared_ptr<Renderer> renderer)
        : m_window(window), m_renderer(renderer)
    {
        init_imgui();
    }

    GuiContext::~GuiContext()
    {
        if (m_initialized)
        {
            Cleanup();
        }
    }

    void GuiContext::init_imgui()
    {
        // Create descriptor pool for ImGui
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VK_CHECK(vkCreateDescriptorPool(m_renderer->GetDevice(), &pool_info, nullptr, &m_imguiDescriptorPool));

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        GLFWwindow* glfwWindow = ServiceLocator::Get<Window>()->GetGLFWWindow();

        ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

        // Configure dynamic rendering for ImGui
        VkFormat colorFormat = m_renderer->GetSwapchainImageFormat();
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;

        // Initialize ImGui for Vulkan with dynamic rendering
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_renderer->GetInstance();
        init_info.PhysicalDevice = m_renderer->GetPhysicalDevice();
        init_info.Device = m_renderer->GetDevice();
        init_info.QueueFamily = m_renderer->GetGraphicsQueueFamily();
        init_info.Queue = m_renderer->GetGraphicsQueue();
        init_info.DescriptorPool = m_imguiDescriptorPool;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

        ImGui_ImplVulkan_Init(&init_info);

        m_initialized = true;
        Logger::Log(LogLevel::INFO, "ImGui initialized successfully");
    }

    void GuiContext::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GuiContext::EndFrame()
    {
        ImGui::Render();
    }

    void GuiContext::RenderToCommandBuffer(VkCommandBuffer cmd)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void GuiContext::Cleanup()
    {
        if (!m_initialized) return;

        vkDeviceWaitIdle(m_renderer->GetDevice());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyDescriptorPool(m_renderer->GetDevice(), m_imguiDescriptorPool, nullptr);

        m_initialized = false;
        Logger::Log(LogLevel::INFO, "ImGui cleaned up");
    }

    void GuiContext::shutdown_imgui()
    {
        Cleanup();
    }
}
