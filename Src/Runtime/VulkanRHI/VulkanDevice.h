#pragma once

#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanPipelineCache.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_structs.hpp"
RHI_NAMESPACE_BEGIN

class VulkanRenderPipeline;
class VulkanDevice
{
public:

private:
    VulkanPhysicalDevice* m_vulkanPhysicalDevice = nullptr;
    const VulkanPhysicalDevice::QueueFamilyIndices* m_queueFamilyIndices  = nullptr;

    vk::SurfaceKHR* m_vkSurfaceKHR = nullptr;
    vk::Device m_vkDevice;
    vk::Queue m_vkGraphicQueue;
    vk::Queue m_vkPresentQueue;

    std::unique_ptr<VulkanSwapchain> m_pVulkanSwapchain;
    std::unique_ptr<VulkanCommandPool> m_pVulkanCmdPool;
    std::unique_ptr<VulkanPipelineCache> m_pVulkanPipelineCache;
public:
    explicit VulkanDevice(VulkanPhysicalDevice* physicalDevice);
    ~VulkanDevice();

    std::vector<vk::PresentModeKHR> GetSurfacePresentMode();
    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormat();
    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities();

    void CreateSwapchainFramebuffer(VulkanRenderPass* renderPass);
    vk::Framebuffer GetSwapchainFramebuffer(int index);
    void ReCreateSwapchain(VulkanRenderPass* renderPass);

    inline vk::Extent2D GetSwapchainExtent() { return m_pVulkanSwapchain->GetSwapchainInfo().imageExtent; }
    inline const VulkanPhysicalDevice::QueueFamilyIndices& GetQueueFamilyIndices() { return *m_queueFamilyIndices; }
    inline vk::SurfaceKHR& GetVkSurface() { return *m_vkSurfaceKHR; }
    inline vk::Device& GetVkDevice() { return m_vkDevice; }
    inline VulkanPhysicalDevice* GetVulkanPhysicalDevice() { return m_vulkanPhysicalDevice; }
    inline VulkanSwapchain* GetPVulkanSwapchain() { return m_pVulkanSwapchain.get(); }
    inline VulkanCommandPool* GetPVulkanCmdPool() { return m_pVulkanCmdPool.get(); }
    inline vk::Queue& GetVkGraphicQueue() { return m_vkGraphicQueue; }
    inline vk::Queue& GetVkPresentQueue() { return m_vkPresentQueue; }
private:
    void setUpQueueCreateInfos(vk::DeviceCreateInfo& createInfo, std::vector<vk::DeviceQueueCreateInfo>& queueInfo);
    void setUpExtensions(vk::DeviceCreateInfo& createInfo, std::vector<const char*>& enabledExtensions);
};

RHI_NAMESPACE_END