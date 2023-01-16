#include "VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <algorithm>
#include <iostream>
#include <string.h>


#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
    VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

RHI_NAMESPACE_USING

VulkanInstance::VulkanInstance(const VulkanInstance::Config& config)
{
    static vk::DynamicLoader  dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    m_config = config;

    vk::InstanceCreateInfo createInfo;
    vk::ApplicationInfo appInfo;
    setUpApplicationInfo(createInfo, appInfo);
    setUpInstanceExtensions(createInfo);
    setUpInstanceLayers(createInfo);
    createVkInstance(createInfo);
    enableDebugValidationLayers();
}

void VulkanInstance::setUpApplicationInfo(vk::InstanceCreateInfo& instanceCreateInfo, vk::ApplicationInfo& appInfo)
{
    appInfo.setPNext(nullptr)
            .setApiVersion(m_config.apiVersion)
            .setPApplicationName(m_config.appName)
            .setApplicationVersion(1)
            .setPEngineName(m_config.engineName)
            .setEngineVersion(1);
    instanceCreateInfo.setPApplicationInfo(&appInfo);
}

void VulkanInstance::setUpInstanceExtensions(vk::InstanceCreateInfo& instanceCreateInfo)
{
    m_instanceInfo.supportedExtensionProps = vk::enumerateInstanceExtensionProperties(nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    // enable instance extensions
    if (!m_config.enabledInstanceExtensions.empty())
    {
        m_instanceInfo.usingExtensions.clear();
        for (const char* enabledExtension : m_config.enabledInstanceExtensions)
        {
            auto it = m_instanceInfo.supportedExtensionProps.begin();
            while (it != m_instanceInfo.supportedExtensionProps.end())
            {
                if (strcmp(enabledExtension, it->extensionName.data()) == 0)
                {
                    break;
                }
                it++;
            }
            if (it != m_instanceInfo.supportedExtensionProps.end())
            {
                m_instanceInfo.usingExtensions.emplace_back(enabledExtension);
            }
            else
            {
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
            }
        }
    }

    if (!m_instanceInfo.usingExtensions.empty())
    {
        if (m_config.validation)
        {
            m_instanceInfo.usingExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // SRS - Dependency when VK_EXT_DEBUG_MARKER is enabled
            m_instanceInfo.usingExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }

    instanceCreateInfo.setPEnabledExtensionNames(m_instanceInfo.usingExtensions);
}


void VulkanInstance::setUpInstanceLayers(vk::InstanceCreateInfo& instanceCreateInfo)
{
    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
	// Note that on Android this layer requires at least NDK r20
    constexpr const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (m_config.validation)
    {
        m_instanceInfo.supportedLayerProps = vk::enumerateInstanceLayerProperties();

        bool validationLayerPresent = false;
        for (auto& property : m_instanceInfo.supportedLayerProps)
        {
            if (strcmp(m_instanceInfo.usingLayers.front(), property.layerName) == 0)
            {
                validationLayerPresent = true;
                break;
            }
        }

        if (validationLayerPresent)
        {
            instanceCreateInfo.setPEnabledLayerNames(m_instanceInfo.usingLayers);
        }
        else
        {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
            m_instanceInfo.usingLayers.clear();
        }
    }
}

void VulkanInstance::createVkInstance(vk::InstanceCreateInfo& instanceCreateInfo)
{
    m_vkInstance = vk::createInstance(instanceCreateInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkInstance);
}


void VulkanInstance::enableDebugValidationLayers()
{
    if (m_config.validation)
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugInfo;
        debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
                .setPfnUserCallback(VulkanInstance::debugUtilsMessengerCallback)
                .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        m_vkDebugUtilMsgExt = m_vkInstance.createDebugUtilsMessengerEXT(debugInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }
}

VulkanInstance::~VulkanInstance()
{
    m_vkInstance.destroyDebugUtilsMessengerEXT(m_vkDebugUtilMsgExt);
    m_vkInstance.destroy();
}



VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // Select prefix depending on flags passed to the callback
    std::string prefix("");

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        prefix = "VERBOSE: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        prefix = "INFO: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        prefix = "WARNING: ";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        prefix = "ERROR: ";
    }


    // Display message to default output (console/logcat)
    std::stringstream debugMessage;
    debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;

#if defined(__ANDROID__)
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOGE("%s", debugMessage.str().c_str());
    } else {
        LOGD("%s", debugMessage.str().c_str());
    }
#else
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << debugMessage.str() << "\n";
    } else {
        std::cout << debugMessage.str() << "\n";
    }
    fflush(stdout);
#endif


    // The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
    // We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
    // If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT 
    return VK_FALSE;
}