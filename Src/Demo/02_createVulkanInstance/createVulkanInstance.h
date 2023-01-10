#pragma once
#include "Demo/01_createwindow/createwindow.h"
#include "vulkan/vulkan_core.h"
#include <memory>


namespace _02
{

class VulkanInstance
{
protected:
    std::unique_ptr<_01::Window> window = nullptr;
    VkInstance instance;
public:
    VulkanInstance();
    virtual ~VulkanInstance();
    virtual void run();
    virtual void initWindow();
    virtual void initVulkan();
    virtual void mainLoop();
    virtual void cleanUp();
protected:
    virtual void createInstance();
};

int createVulkanInstance();
}