#pragma once
#include "Runtime/Platform/PlatformWindow.h"
#include "VulkanRHI.h"
#include "vulkan/vulkan_handles.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
RHI_NAMESPACE_BEGIN

class VulkanPhysicalDevice
{
public:

    struct Config
    {
        platform::PlatformWindow* window;
    };

    struct PhysicalDeviceInfo
    {
        vk::PhysicalDeviceProperties deviceProps;
        vk::PhysicalDeviceFeatures deviceFeatures;
        vk::PhysicalDeviceMemoryProperties deviceMemoryProps;
        std::vector<vk::QueueFamilyProperties> deviceQueueFamilyProps;
        std::vector<vk::ExtensionProperties> deviceExtensionProps;
        std::vector<std::string> supportedExtensions;
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphic;
        std::optional<uint32_t> present;
        operator bool() const { return graphic.has_value() && present.has_value(); }
    };
private:
    PhysicalDeviceInfo m_physicalDeviceInfo;

    VulkanInstance* m_pVulkanInstance;
    platform::PlatformWindow* m_window;

    vk::PhysicalDevice m_vkPhysicalDevice;
public:
    explicit VulkanPhysicalDevice(const Config& config, VulkanInstance* instance);
    ~VulkanPhysicalDevice();

    inline vk::PhysicalDevice& GetVkPhysicalDevice() { return m_vkPhysicalDevice; }
    inline const PhysicalDeviceInfo& GetPhysicalDeviceInfo() { return m_physicalDeviceInfo; }

    vk::SurfaceKHR CreateVkSurface();
    void DestroyVkSurface(vk::SurfaceKHR& surface);

    QueueFamilyIndices QueryQueueFamilyIndices(vk::SurfaceKHR& surface);
    bool SupportExtension(const std::string& extension);

    inline int GetWindowWidth() { return m_window ? m_window->GetWindowSetting().width : 0; }
    inline int GetWindowHeight() { return m_window ? m_window->GetWindowSetting().height : 0; }
private:

};

RHI_NAMESPACE_END