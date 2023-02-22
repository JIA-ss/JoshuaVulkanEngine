#pragma once

#include "Runtime/Platform/PlatformInputMonitor.h"
#include "backends/imgui_impl_glfw.h"

namespace platform {

class WindowsInputMonitor : public PlatformInputMonitor
{
public:
    void Init(PlatformWindow* window) override;
    void UnInit(PlatformWindow* window) override;

private:
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


};

}