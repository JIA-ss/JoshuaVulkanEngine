#pragma once

#include <vulkan/vulkan.hpp>
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
    std::vector<vk::Image> m_vkImages;
    std::vector<vk::ImageView> m_vkImageViews;
    std::unique_ptr<VulkanImageResource> m_pVulkanDepthImage;
    std::unique_ptr<VulkanImageResource> m_pVulkanResolveColorImage;
    std::vector<vk::Framebuffer> m_vkFramebuffers;

    //VulkanInstance* m_pVulkanInstance;
    //VulkanPhysicalDevice* m_pVulkanPhysicalDevice;
    VulkanDevice* m_pVulkanDevice;

    vk::SwapchainKHR m_vkSwapchain;
    SwapchainInfo m_swapchainInfo;
public:
    VulkanSwapchain(VulkanDevice* device);
    ~VulkanSwapchain();
    inline const SwapchainInfo& GetSwapchainInfo() { return m_swapchainInfo; }
    inline vk::SwapchainKHR& GetSwapchain() { return m_vkSwapchain; }
    inline std::vector<vk::Framebuffer>& GetFramebuffers() { return m_vkFramebuffers; }
    inline vk::Framebuffer& GetFramebuffer(int index) { assert(index >= 0 && index < m_vkFramebuffers.size()); return m_vkFramebuffers[index]; }

    void CreateFrameBuffers(VulkanRenderPass* renderpass);
private:
    void queryInfo();
    void getImages();
    void createImageViews();
    void createDepthAndResolveColorImage();

};

RHI_NAMESPACE_END