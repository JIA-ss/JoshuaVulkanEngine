#include "WindowsWindow.h"
#include <GLFW/glfw3.h>

platform::WindowsWindow::~WindowsWindow()
{
    assert(!m_glfwWindowPtr);
}

void platform::WindowsWindow::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_glfwWindowPtr = glfwCreateWindow(m_setting.width, m_setting.height, m_setting.name, nullptr, nullptr);
}

void platform::WindowsWindow::Destroy()
{
    if (m_glfwWindowPtr)
    {
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