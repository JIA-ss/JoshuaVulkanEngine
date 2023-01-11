#include "createwindow.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

using namespace _01;

Window::Window(const WindowSetting& _setting) : setting(_setting)
{

}

Window::~Window()
{
    if (glfwWindow)
    {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }
}

void Window::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindow = glfwCreateWindow(setting.width, setting.height, setting.name, nullptr, nullptr);
}

bool Window::shouldClose()
{
    return !glfwWindow || glfwWindowShouldClose(glfwWindow);
}

std::vector<const char*> Window::getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
}

std::function<VkSurfaceKHR(vk::Instance)> Window::getCreateSurfaceFunc(VkAllocationCallbacks* allocator)
{
    return [=](vk::Instance instance)
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, glfwWindow, allocator, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("glfw create window surface failed");
        }
        return surface;
    };
}

int _01::createWindow()
{
    Window window({800, 600, "01 Create Vulkan Window"});
    window.initWindow();
    while (!window.shouldClose())
    {
        glfwPollEvents();
    }

    return 0;
}