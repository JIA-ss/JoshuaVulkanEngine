#include "swapchain.h"
#include "CppDemo/01_context/context.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <algorithm>
#include <array>
#include <stdint.h>

namespace cpp_demo {

Swapchain::Swapchain(int windowWidth, int windowHeight)
{
    queryInfo(windowWidth, windowHeight);


    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setClipped(true)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setSurface(Context::GetInstance().GetSurface())
                .setImageColorSpace(m_swapchainInfo.format.colorSpace)
                .setImageFormat(m_swapchainInfo.format.format)
                .setImageExtent(m_swapchainInfo.imageExtent)
                .setMinImageCount(m_swapchainInfo.imageCount)
                .setPresentMode(m_swapchainInfo.present);

    auto queueIndices = Context::GetInstance().GetQueueFamilyIndices();
    if (queueIndices.presentQueue.value() == queueIndices.graphicsQueue.value())
    {
        createInfo.setQueueFamilyIndices(queueIndices.graphicsQueue.value())
                    .setImageSharingMode(vk::SharingMode::eExclusive);
    }
    else
    {
        std::array<uint32_t, 2> indices { queueIndices.graphicsQueue.value(), queueIndices.presentQueue.value()};
        createInfo.setQueueFamilyIndices(indices)
                    .setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    m_vkSwapchain = Context::GetInstance().GetDevice().createSwapchainKHR(createInfo);

    getImages();
    createImageViews();
}

Swapchain::~Swapchain()
{
    auto& device = Context::GetInstance().GetDevice();
    for (auto& framebuffer: m_vkFramebuffers)
    {
        device.destroyFramebuffer(framebuffer);
    }
    for (auto& imageView : m_vkImageViews)
    {
        device.destroyImageView(imageView);
    }
    device.destroySwapchainKHR(m_vkSwapchain);
}

void Swapchain::queryInfo(int windowWidth, int windowHeight)
{
    m_swapchainInfo.present = vk::PresentModeKHR::eFifo;

    auto& physicalDevice = Context::GetInstance().GetPhysicalDevice();
    auto& surface = Context::GetInstance().GetSurface();

    auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
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

    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    m_swapchainInfo.imageCount = std::clamp<uint32_t>(2, capabilities.minImageCount, capabilities.maxImageCount);

    m_swapchainInfo.imageExtent.width = std::clamp<uint32_t>(windowWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    m_swapchainInfo.imageExtent.height = std::clamp<uint32_t>(windowHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    m_swapchainInfo.transform = capabilities.currentTransform;

    auto presents = physicalDevice.getSurfacePresentModesKHR(surface);
    for(const auto& present : presents)
    {
        if (present == vk::PresentModeKHR::eMailbox)
        {
            m_swapchainInfo.present = present;
            break;
        }
    }
}

void Swapchain::getImages()
{
    m_vkImages = Context::GetInstance().GetDevice().getSwapchainImagesKHR(m_vkSwapchain);
}

void Swapchain::createImageViews()
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
        m_vkImageViews[i] = Context::GetInstance().GetDevice().createImageView(createInfo);
    }
}

void Swapchain::CreateFrameBuffers(int windowWidth, int windowHeight)
{
    m_vkFramebuffers.resize(m_vkImages.size());
    for (int i = 0; i < m_vkImages.size(); i++)
    {
        vk::FramebufferCreateInfo createInfo;
        createInfo.setAttachments(m_vkImageViews[i])
                    .setWidth(windowWidth)
                    .setHeight(windowHeight)
                    .setRenderPass(Context::GetInstance().GetRenderProcess().GetRenderPass())
                    .setLayers(1);
        m_vkFramebuffers[i] = Context::GetInstance().GetDevice().createFramebuffer(createInfo);
    }
}

}