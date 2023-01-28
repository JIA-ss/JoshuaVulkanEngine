#pragma once
#include "Runtime/Platform/PlatformWindow.h"
#include "VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <array>
RHI_NAMESPACE_BEGIN

class VulkanPhysicalDevice
{
public:

    struct Config
    {
        platform::PlatformWindow* window;
        std::optional<vk::PhysicalDeviceFeatures> requiredFeatures;
        std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

    struct PhysicalDeviceInfo
    {
        vk::PhysicalDeviceProperties deviceProps;
        //vk::PhysicalDeviceFeatures deviceFeatures;
        //vk::PhysicalDeviceMemoryProperties deviceMemoryProps;
        //std::vector<vk::QueueFamilyProperties> deviceQueueFamilyProps;
        //std::vector<vk::ExtensionProperties> deviceExtensionProps;
        std::vector<std::string> supportedExtensions;
        vk::SampleCountFlagBits maxUsableSampleCount;
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphic;
        std::optional<uint32_t> present;
        operator bool() const { return graphic.has_value() && present.has_value(); }
    };
private:
    PhysicalDeviceInfo m_physicalDeviceInfo;
    Config m_config;
    QueueFamilyIndices m_queueFamilyIndices;

    VulkanInstance* m_pVulkanInstance;

    vk::PhysicalDevice m_vkPhysicalDevice;
    vk::SurfaceKHR m_vkSurface;
public:
    explicit VulkanPhysicalDevice(const Config& config, VulkanInstance* instance);
    ~VulkanPhysicalDevice();

    inline const Config& GetConfig() { return m_config; }
    inline const PhysicalDeviceInfo& GetPhysicalDeviceInfo() { return m_physicalDeviceInfo; }
    inline const QueueFamilyIndices* GetPQueueFamilyIndices() { return &m_queueFamilyIndices; }
    inline vk::PhysicalDevice& GetVkPhysicalDevice() { return m_vkPhysicalDevice; }
    inline vk::SurfaceKHR* GetPVkSurface() { return &m_vkSurface; }
    bool SupportExtension(const std::string& extension);

    vk::Format QuerySupportedDepthFormat();

    inline platform::PlatformWindow* GetPWindow() { return m_config.window; }
    inline int GetWindowWidth() { return m_config.window ? m_config.window->GetWindowSetting().width : 0; }
    inline int GetWindowHeight() { return m_config.window ? m_config.window->GetWindowSetting().height : 0; }
private:
    void pickUpDevice(const std::vector<vk::PhysicalDevice>& devices);
    bool queryQueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, QueueFamilyIndices& indices);
    bool checkSupportExtension(const vk::PhysicalDevice& device);
    bool checkSupportSwapchain(const vk::PhysicalDevice& device);
    bool checkSupportFeatures(const vk::PhysicalDevice& device);
    void queryDeviceInfo();
    void createVkSurface();
    void destroyVkSurface();
    vk::Format querySupportFormat(const std::vector<vk::Format>& candidates, const vk::ImageTiling& imgTiling, const vk::FormatFeatureFlagBits& feature);
};

RHI_NAMESPACE_END