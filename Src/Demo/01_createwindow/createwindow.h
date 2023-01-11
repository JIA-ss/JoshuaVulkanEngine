#pragma once

#include <functional>
#include <iostream>
#include <imgui.h>
#include "Runtime/Base/Guid.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
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

    std::vector<const char*> getRequiredInstanceExtensions();
    std::function<VkSurfaceKHR(vk::Instance)> getCreateSurfaceFunc(VkAllocationCallbacks* allocator = nullptr);
    const WindowSetting& getWindowSetting() { return setting; }
protected:
    GLFWwindow* glfwWindow = nullptr;
    WindowSetting setting;
};

int createWindow();
}