#pragma once

#include "Runtime/Platform/PlatformWindow.h"
#include "backends/imgui_impl_glfw.h"
#include "vulkan/vulkan.hpp"

namespace platform {

class WindowsWindow : public PlatformWindow
{
public:
protected:
    GLFWwindow* m_glfwWindowPtr = nullptr;
public:
    WindowsWindow(const PlatformWindowSetting& setting) : PlatformWindow(setting), m_glfwWindowPtr(nullptr) {}
    ~WindowsWindow() override;

    void Init() override;
    void Destroy() override;
    bool ShouldClose() override;
    vk::SurfaceKHR CreateSurface(RHI::VulkanInstance* instance) override;
    void DestroySurface(RHI::VulkanInstance* instance, vk::SurfaceKHR* surface) override;
    void* GetRawHandler() override { return m_glfwWindowPtr; }
    std::vector<const char*> GetRequiredExtensions() override;
};

}