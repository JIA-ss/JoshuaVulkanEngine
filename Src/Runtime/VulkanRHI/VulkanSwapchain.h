#pragma once

#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_enums.hpp"

RHI_NAMESPACE_BEGIN
class VulkanDevice;
class VulkanRenderPipeline;
class VulkanSwapchain
{
public:
    struct SwapchainInfo
    {
        vk::Extent2D imageExtent;
        uint32_t imageCount;
        vk::SurfaceFormatKHR format = vk::SurfaceFormatKHR{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
        vk::SurfaceTransformFlagsKHR transform;
        vk::PresentModeKHR present = vk::PresentModeKHR::eFifo;
    };
private:
    std::vector<VulkanImageResource::Native> m_nativePresentImages;
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_pPresentFramebuffers;

    std::unique_ptr<VulkanImageResource> m_pVulkanDepthImage;
    std::unique_ptr<VulkanImageResource> m_pVulkanSuperSamplerColorImage;

    //VulkanInstance* m_pVulkanInstance;
    //VulkanPhysicalDevice* m_pVulkanPhysicalDevice;
    VulkanDevice* m_pVulkanDevice;

    vk::SwapchainKHR m_vkSwapchain;
    SwapchainInfo m_swapchainInfo;
public:
    VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain();
    inline const SwapchainInfo& GetSwapchainInfo() const { return m_swapchainInfo; }
    inline const vk::SwapchainKHR& GetSwapchain() const { return m_vkSwapchain; }

    void GetVulkanPresentColorAttachment(int idx, VulkanFramebuffer::Attachment& attachment) const;
    inline const VulkanImageResource::Native& GetVulkanPresentImage(int idx) const { return m_nativePresentImages[idx]; }
    inline const std::vector<VulkanImageResource::Native>& GetVulkanPresentImages() const { return m_nativePresentImages; }
private:
    void queryInfo();
    void getImages();
    void createImageViews();

};

RHI_NAMESPACE_END