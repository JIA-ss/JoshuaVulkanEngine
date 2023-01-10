#include "createwindow.h"
#include <GLFW/glfw3.h>

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

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported" << std::endl;
}

bool Window::shouldClose()
{
    return !glfwWindow || glfwWindowShouldClose(glfwWindow);
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