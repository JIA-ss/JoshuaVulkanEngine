#pragma once

#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanPipelineCache.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
RHI_NAMESPACE_BEGIN

class VulkanRenderPipeline;
class VulkanDevice
{

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
    VulkanDescriptorSetLayoutPresets m_VulkanDescriptorSetLayoutPresets;

    std::map<std::string, std::unique_ptr<VulkanFramebuffer>> m_pVulkanFramebuffers;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_pPresentVulkanFramebuffers;
public:
    explicit VulkanDevice(VulkanPhysicalDevice* physicalDevice);
    ~VulkanDevice();

    std::vector<vk::PresentModeKHR> GetSurfacePresentMode();
    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormat();
    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities();

    VulkanFramebuffer* CreateVulkanFramebuffer(const char* name, VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer = 1,
        const std::vector<VulkanFramebuffer::Attachment>& attachments = {}
    );
    void DestroyVulkanFramebuffer(const char* name);
    VulkanFramebuffer* GetVulkanFramebuffer(const char* name);

    void CreateVulkanPresentFramebuffer(VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer = 1,
        const std::vector<VulkanFramebuffer::Attachment>& attachments = {}, int presentImageIdx = 0
    );
    inline VulkanFramebuffer* GetVulkanPresentFramebuffer(int idx) { return m_pPresentVulkanFramebuffers[idx].get(); }

    void ReCreateSwapchain(VulkanRenderPass* renderPass,
        uint32_t width, uint32_t height, uint32_t layer,
        std::vector<VulkanFramebuffer::Attachment>& attachments, int presentImageIdx);
    VulkanDescriptorSetLayoutPresets& GetDescLayoutPresets() { return m_VulkanDescriptorSetLayoutPresets; }

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