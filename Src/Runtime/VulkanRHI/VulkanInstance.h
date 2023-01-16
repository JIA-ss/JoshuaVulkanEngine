#pragma once

#include "VulkanRHI.h"
#include <vector>
#include <stdint.h>
#include <vulkan/vulkan.hpp>


RHI_NAMESPACE_BEGIN

class VulkanInstance
{
public:
    struct Config
    {
        bool validation = true;
        const char* appName = nullptr;
        const char* engineName = nullptr;
        uint32_t apiVersion = VK_API_VERSION_1_2;
        std::vector<const char*> enabledInstanceExtensions;
    };

    struct InstanceInfo
    {
        std::vector<vk::ExtensionProperties> supportedExtensionProps;
        std::vector<vk::LayerProperties> supportedLayerProps;

        std::vector<const char*> usingExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
        std::vector<const char*> usingLayers = { "VK_LAYER_KHRONOS_validation" };
    };
protected:
    Config m_config;
    InstanceInfo m_instanceInfo;

    vk::Instance m_vkInstance;
    vk::DebugUtilsMessengerEXT m_vkDebugUtilMsgExt;
public:
    explicit VulkanInstance(const Config& config);
    ~VulkanInstance();

    inline vk::Instance& GetVkInstance() { return m_vkInstance; }
    inline Config& GetConfig() { return m_config; }
    inline InstanceInfo& GetInstaceInfo() { return m_instanceInfo; }


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

private:

    void setUpApplicationInfo(vk::InstanceCreateInfo& instanceCreateInfo, vk::ApplicationInfo& appInfo);
    void setUpInstanceExtensions(vk::InstanceCreateInfo& instanceCreateInfo);
    void setUpInstanceLayers(vk::InstanceCreateInfo& instanceCreateInfo);
    void createVkInstance(vk::InstanceCreateInfo& instanceCreateInfo);
    void enableDebugValidationLayers();
};

RHI_NAMESPACE_END