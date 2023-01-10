#pragma once

#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "vulkan/vulkan_core.h"
#include <optional>
#include <stdint.h>
#include <vector>
namespace _03
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    inline bool isComplete()
    {
        return graphicsFamily.has_value();
    }
};

class VulkanInstance : public _02::VulkanInstance
{
protected:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger;
public:
    void initVulkan() override;
    void cleanUp() override;
    void createInstance() override;
protected:
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    std::vector<const char*> getRequiredExtensions();
};

int physicalDeviceAndQueue();
}