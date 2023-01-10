#include "createVulkanInstance.h"
#include "Demo/01_createwindow/createwindow.h"
#include "vulkan/vulkan_core.h"
#include <memory>

// 01_instance_creation.cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const int WIDTH = 800;
const int HEIGHT = 600;

using namespace _02;

VulkanInstance::VulkanInstance()
{

}

VulkanInstance::~VulkanInstance()
{

}

void VulkanInstance::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}

void VulkanInstance::initWindow()
{
    window = std::make_unique<_01::Window>(_01::WindowSetting{WIDTH, HEIGHT, "02 Create Vulkan Instance"});
    window->initWindow();
}

void VulkanInstance::initVulkan()
{
    createInstance();
}

void VulkanInstance::mainLoop()
{
    while (window && !window->shouldClose())
    {
        glfwPollEvents();
    }
}
void VulkanInstance::cleanUp()
{
    vkDestroyInstance(instance, nullptr);
    window.reset();
}


void VulkanInstance::createInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }
}

int _02::createVulkanInstance()
{
    VulkanInstance instance;
    try {
        instance.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}