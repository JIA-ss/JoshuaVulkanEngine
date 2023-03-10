#include "WindowsWindow.h"
#include "Runtime/Platform/Windows/WindowsInputMonitor.h"
#include "Runtime/Platform/Windows/WindowsWindow.h"
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace glfwUtil{

GLFWmonitor* GetValidMonitor(int id)
{
    int monitorCount=-1;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor* monitor = id < monitorCount ? monitors[id] : monitors[0];

    return monitor;
}

}

platform::WindowsWindow::~WindowsWindow()
{
    assert(!m_glfwWindowPtr);
}

void platform::WindowsWindow::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);


    m_glfwWindowPtr = glfwCreateWindow(m_setting.width, m_setting.height, m_setting.name, nullptr, nullptr);
    glfwSetWindowPos(m_glfwWindowPtr, m_setting.pos.x, m_setting.pos.y);

    if (m_setting.maximization)
    {
        glfwMaximizeWindow(m_glfwWindowPtr);
    }

    glfwSetWindowUserPointer(m_glfwWindowPtr, this);
    glfwSetFramebufferSizeCallback(m_glfwWindowPtr, platform::WindowsWindow::frameBufferSizeChanged);


    m_inputMonitor.reset(new WindowsInputMonitor());
    m_inputMonitor->Init(this);



    m_inputMonitor->AddKeyboardPressedCallback(Keyboard::Key::F11, [&](){
            SetMaximizationMode();
    });

    AddFrameBufferSizeChangedCallback([this](int width, int height)
    {
        if (width != 0 && height != 0)
        {
            m_setting.width = width;
            m_setting.height = height;
        }
    });



}

void platform::WindowsWindow::Destroy()
{
    m_inputMonitor->UnInit(this);
    m_inputMonitor.reset();

    if (m_glfwWindowPtr)
    {
        glfwSetWindowUserPointer(m_glfwWindowPtr, nullptr);
        glfwDestroyWindow(m_glfwWindowPtr);
        glfwTerminate();
    }
    m_glfwWindowPtr = nullptr;
}

bool platform::WindowsWindow::ShouldClose()
{
    return !m_glfwWindowPtr || glfwWindowShouldClose(m_glfwWindowPtr);
}

vk::SurfaceKHR platform::WindowsWindow::CreateSurface(RHI::VulkanInstance* instance)
{
    if (instance == nullptr)
    {
        return {};
    }
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), m_glfwWindowPtr, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("glfw create window surface failed");
    }
    return surface;
}

void platform::WindowsWindow::DestroySurface(RHI::VulkanInstance* instance, vk::SurfaceKHR* surface)
{
    if (!instance || !surface)
    {
        return;
    }
    instance->GetVkInstance().destroySurfaceKHR(*surface);
}

std::vector<const char*> platform::WindowsWindow::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
}


void platform::WindowsWindow::SetMaximizationMode()
{
    m_setting.maximization = glfwGetWindowAttrib(m_glfwWindowPtr, GLFW_MAXIMIZED);
    if (m_setting.maximization)
    {
        glfwMaximizeWindow(m_glfwWindowPtr);
    }
    else
    {
        glfwRestoreWindow(m_glfwWindowPtr);
    }
}



void platform::WindowsWindow::frameBufferSizeChanged(GLFWwindow* window, int width, int height)
{
    WindowsWindow* platformWindow = (WindowsWindow*)glfwGetWindowUserPointer(window);
    if (platformWindow == nullptr)
    {
        return;
    }
    for (auto func : platformWindow->m_frameBufferSizeChangedCallbacks)
    {
        func(width, height);
    }
}

void platform::WindowsWindow::AddFrameBufferSizeChangedCallback(std::function<void(int, int)> func)
{
    m_frameBufferSizeChangedCallbacks.emplace_back(func);
}

void platform::WindowsWindow::WaitIfMinimization()
{
    while (m_setting.height == 0 || m_setting.width == 0)
    {
        glfwWaitEvents();
    }
}

void platform::WindowsWindow::PollWindowEvent()
{
    glfwPollEvents();
}