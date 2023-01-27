#include "VulkanSwapchain.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "vulkan/vulkan_structs.hpp"
#include <iostream>
RHI_NAMESPACE_USING


VulkanSwapchain::VulkanSwapchain(VulkanDevice* device)
    : m_pVulkanDevice(device)
{
    std::cout << "=== === === VulkanSwapchain Construct Begin === === ===" << std::endl;


    queryInfo();


    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setClipped(true)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setSurface(m_pVulkanDevice->GetVkSurface())
                .setImageColorSpace(m_swapchainInfo.format.colorSpace)
                .setImageFormat(m_swapchainInfo.format.format)
                .setImageExtent(m_swapchainInfo.imageExtent)
                .setMinImageCount(m_swapchainInfo.imageCount)
                .setPresentMode(m_swapchainInfo.present);

    auto queueIndices = m_pVulkanDevice->GetQueueFamilyIndices();
    if (queueIndices.present.value() == queueIndices.graphic.value())
    {
        createInfo.setQueueFamilyIndices(queueIndices.graphic.value())
                    .setImageSharingMode(vk::SharingMode::eExclusive);
    }
    else
    {
        std::array<uint32_t, 2> indices { queueIndices.graphic.value(), queueIndices.present.value()};
        createInfo.setQueueFamilyIndices(indices)
                    .setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    m_vkSwapchain = m_pVulkanDevice->GetVkDevice().createSwapchainKHR(createInfo);

    getImages();
    createImageViews();

    std::cout << "=== === === VulkanSwapchain Construct End === === ===" << std::endl;
}

VulkanSwapchain::~VulkanSwapchain()
{
    for (auto& imgView : m_vkImageViews)
    {
        m_pVulkanDevice->GetVkDevice().destroyImageView(imgView);
    }
    for (auto& framebuffer: m_vkFramebuffers)
    {
        m_pVulkanDevice->GetVkDevice().destroyFramebuffer(framebuffer);
    }
    m_pVulkanDevice->GetVkDevice().destroySwapchainKHR(m_vkSwapchain);
}

void VulkanSwapchain::queryInfo()
{
    int windowWidth = m_pVulkanDevice->GetVulkanPhysicalDevice()->GetWindowWidth();
    int windowHeight = m_pVulkanDevice->GetVulkanPhysicalDevice()->GetWindowHeight();

    auto formats = m_pVulkanDevice->GetSurfaceFormat();
    m_swapchainInfo.format = formats.front();
    for (const auto& format : formats)
    {
        if (format.format == vk::Format::eR8G8B8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            m_swapchainInfo.format = format;
            break;
        }
    }

    auto capabilities = m_pVulkanDevice->GetSurfaceCapabilities();
    m_swapchainInfo.imageCount = std::clamp<uint32_t>(2, capabilities.minImageCount, capabilities.maxImageCount);
    m_swapchainInfo.imageExtent.width = std::clamp<uint32_t>(windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_swapchainInfo.imageExtent.height = std::clamp<uint32_t>(windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    m_swapchainInfo.transform = capabilities.currentTransform;

    auto presents = m_pVulkanDevice->GetSurfacePresentMode();
    for(const auto& present : presents)
    {
        if (present == vk::PresentModeKHR::eMailbox)
        {
            m_swapchainInfo.present = present;
            break;
        }
    }
}

void VulkanSwapchain::getImages()
{
    m_vkImages = m_pVulkanDevice->GetVkDevice().getSwapchainImagesKHR(m_vkSwapchain);
}

void VulkanSwapchain::createImageViews()
{
    m_vkImageViews.resize(m_vkImages.size());
    for(int i = 0; i < m_vkImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo;
        vk::ComponentMapping mapping;
        vk::ImageSubresourceRange range;
        range.setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
                .setAspectMask(vk::ImageAspectFlagBits::eColor);

        createInfo.setImage(m_vkImages[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setComponents(mapping)
                    .setFormat(m_swapchainInfo.format.format)
                    .setSubresourceRange(range);
        m_vkImageViews[i] = m_pVulkanDevice->GetVkDevice().createImageView(createInfo);
    }
}

void VulkanSwapchain::CreateFrameBuffers(VulkanRenderPipeline* pipeline)
{
    int windowWidth = m_pVulkanDevice->GetVulkanPhysicalDevice()->GetWindowWidth();
    int windowHeight = m_pVulkanDevice->GetVulkanPhysicalDevice()->GetWindowHeight();

    m_vkFramebuffers.resize(m_vkImageViews.size());
    std::vector<vk::FramebufferCreateInfo> createInfos;
    for (int i = 0; i < m_vkImageViews.size(); i++)
    {
        vk::FramebufferCreateInfo createInfo;
        createInfo.setAttachments(m_vkImageViews[i])
                    .setWidth(windowWidth)
                    .setHeight(windowHeight)
                    .setRenderPass(pipeline->GetVkRenderPass())
                    .setLayers(1)
                    .setAttachmentCount(1);
        m_vkFramebuffers[i] = m_pVulkanDevice->GetVkDevice().createFramebuffer(createInfo);
    }
    //m_vkFramebuffers = m_pVulkanDevice->GetVkDevice().createFramebuffer(createInfos);
}
