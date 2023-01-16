#pragma once

#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
RHI_NAMESPACE_BEGIN

class VulkanDevice
{
public:
    struct Config
    {
        std::optional<vk::PhysicalDeviceFeatures> enableFeatures;
        vk::QueueFlags requestedQueueTypes;
        std::vector<const char*> enableExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
private:
    Config m_config;

    VulkanPhysicalDevice* m_vulkanPhysicalDevice = nullptr;
    VulkanPhysicalDevice::QueueFamilyIndices m_queueFamilyIndices;

    vk::SurfaceKHR m_vkSurfaceKHR;
    vk::Device m_vkDevice;
    vk::Queue m_vkGraphicQueue;
    vk::Queue m_vkPresentQueue;

    std::unique_ptr<VulkanSwapchain> m_pVulkanSwapchain;
public:
    explicit VulkanDevice(const Config& config, VulkanPhysicalDevice* physicalDevice);
    ~VulkanDevice();

    std::vector<vk::PresentModeKHR> GetSurfacePresentMode();
    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormat();
    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities();

    inline const VulkanPhysicalDevice::QueueFamilyIndices& GetQueueFamilyIndices() { return m_queueFamilyIndices; }
    inline vk::SurfaceKHR& GetVkSurface() { return m_vkSurfaceKHR; }
    inline vk::Device& GetVkDevice() { return m_vkDevice; }
    inline VulkanPhysicalDevice* GetVulkanPhysicalDevice() { return m_vulkanPhysicalDevice; }
private:
    void setUpQueueCreateInfos(vk::DeviceCreateInfo& createInfo, std::vector<vk::DeviceQueueCreateInfo>& queueInfo);
    void setUpExtensions(vk::DeviceCreateInfo& createInfo);
};

RHI_NAMESPACE_END