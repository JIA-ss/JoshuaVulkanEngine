#include "Editor.h"
#include "Runtime/Platform/PlatformWindow.h"
#include <GLFW/glfw3.h>

namespace editor {

std::unique_ptr<platform::PlatformWindow> g_platformWindow = nullptr;
platform::PlatformWindow* GetWindow() { return g_platformWindow.get(); }
void StartUp()
{
    g_platformWindow = platform::CreatePlatformWindow(1920, 1080, "VulkanEngine");
    g_platformWindow->Init();

}

void Run()
{
    while (g_platformWindow && !g_platformWindow->ShouldClose())
    {
        glfwPollEvents();
    }
}

void ShutDown()
{
    g_platformWindow->Destroy();
    g_platformWindow.reset();
}

}