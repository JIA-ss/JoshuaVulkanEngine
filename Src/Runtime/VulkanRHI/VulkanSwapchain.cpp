#include "VulkanSwapchain.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "vulkan/vulkan_enums.hpp"
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
    createDepthAndResolveColorImage();

    std::cout << "=== === === VulkanSwapchain Construct End === === ===" << std::endl;
}

VulkanSwapchain::~VulkanSwapchain()
{
    m_pVulkanDepthImage.reset();
    m_pVulkanSuperSamplerColorImage.reset();
    for (auto& imgNativef : m_nativePresentImages)
    {
        if (imgNativef.vkImageView)
        {
            m_pVulkanDevice->GetVkDevice().destroyImageView(imgNativef.vkImageView.value());
        }
    }

    m_pVulkanDevice->GetVkDevice().destroySwapchainKHR(m_vkSwapchain);
}


void VulkanSwapchain::GetVulkanPresentColorAttachment(int idx, VulkanFramebuffer::Attachment& attachment) const
{
    attachment.resource = m_nativePresentImages[idx];
    attachment.resourceFinalLayout = vk::ImageLayout::ePresentSrcKHR;
    attachment.attachmentLayout = vk::ImageLayout::eColorAttachmentOptimal;
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
    m_swapchainInfo.imageCount = std::clamp<uint32_t>(MAX_FRAMES_IN_FLIGHT, capabilities.minImageCount, capabilities.maxImageCount);
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
    auto vkImages = m_pVulkanDevice->GetVkDevice().getSwapchainImagesKHR(m_vkSwapchain);
    m_nativePresentImages.resize(vkImages.size());
    for (int i = 0; i < vkImages.size(); i++)
    {
        m_nativePresentImages[i].vkImage = vkImages[i];
    }
}

void VulkanSwapchain::createImageViews()
{

    VulkanImageResource::Config config;
    config.subresourceRange
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            ;
    config.imageViewType = vk::ImageViewType::e2D;
    config.format = m_swapchainInfo.format.format;

    for(int i = 0; i < m_nativePresentImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo;
        vk::ComponentMapping mapping;

        createInfo.setImage(m_nativePresentImages[i].vkImage.value())
                    .setViewType(config.imageViewType)
                    .setComponents(mapping)
                    .setFormat(config.format)
                    .setSubresourceRange(config.subresourceRange);
        m_nativePresentImages[i].vkImageView = m_pVulkanDevice->GetVkDevice().createImageView(createInfo);
        m_nativePresentImages[i].config = config;
    }
}

void VulkanSwapchain::createDepthAndResolveColorImage()
{
    VulkanImageResource::Config config;
    config.format = m_pVulkanDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    config.sampleCount = m_pVulkanDevice->GetVulkanPhysicalDevice()->GetSampleCount();
    config.extent = vk::Extent3D{ m_swapchainInfo.imageExtent, 1};
    config.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    if (m_pVulkanDevice->GetVulkanPhysicalDevice()->HasStencilComponent(config.format))
    {
        config.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        // config.subresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
    }
    else
    {
        config.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        // config.subresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eDepth);
    }
    m_pVulkanDepthImage.reset(new VulkanImageResource(m_pVulkanDevice, vk::MemoryPropertyFlagBits::eDeviceLocal, config));
    // m_pVulkanDepthImage->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    if (m_pVulkanDevice->GetVulkanPhysicalDevice()->IsUsingMSAA())
    {
        config.format = m_swapchainInfo.format.format;
        config.imageUsage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
        config.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        m_pVulkanSuperSamplerColorImage.reset(new VulkanImageResource(m_pVulkanDevice, vk::MemoryPropertyFlagBits::eDeviceLocal, config));
        // m_pVulkanSuperSamplerColorImage->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    }
}


