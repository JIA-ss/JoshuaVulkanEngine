#pragma once

#include "Runtime/Platform/PlatformWindow.h"
#include <map>
#include "backends/imgui_impl_glfw.h"
#include "vulkan/vulkan.hpp"

namespace platform {

class WindowsWindow : public PlatformWindow
{
public:
protected:
    GLFWwindow* m_glfwWindowPtr = nullptr;

    std::vector<std::function<void(unsigned int, unsigned int)>> m_frameBufferSizeChangedCallbacks;
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
    void AddFrameBufferSizeChangedCallback(std::function<void(int, int)> func) override;
    void WaitIfMinimization() override;
    void PollWindowEvent() override;
private:
    static void frameBufferSizeChanged(GLFWwindow* window, int, int);
};

}