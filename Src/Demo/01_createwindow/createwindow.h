#pragma once

#include <iostream>
#include <imgui.h>
#include "Runtime/Base/Guid.h"


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace _01
{

struct WindowSetting
{
    unsigned int width;
    unsigned int height;
    const char* name;
};

class Window
{
public:
    explicit Window(const WindowSetting& setting);
    virtual ~Window();

    void initWindow();
    const GLFWwindow* getInternalWindow() { return glfwWindow; }
    bool shouldClose();
protected:
    GLFWwindow* glfwWindow = nullptr;
    WindowSetting setting;
};

int createWindow();
}