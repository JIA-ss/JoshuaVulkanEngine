#include "VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <stdexcept>
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanImage::VulkanImage(
    VulkanDevice* device,
    std::shared_ptr<util::texture::RawData> rawData,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags memProps,
    vk::ImageLayout layout
)
    : m_vulkanDevice(device)
    , m_pRawData(rawData)
    , m_vkFormat(format)
    , m_vkImageTiling(tiling)
    , m_vkImageUsageFlags(usage)
    , m_vkImageLayout(layout)
{

    createImage();
    allocDeviceMemory(memProps);
    createImageView();
    createSampler();
    createStagingBuffer();

    UploadImageToGPU();
}

VulkanImage::~VulkanImage()
{

    m_pVulkanStagingBuffer.reset();
    m_pRawData = nullptr;

    m_vulkanDevice->GetVkDevice().destroySampler(m_vkSampler);
    m_vulkanDevice->GetVkDevice().destroyImageView(m_vkImageView);
    m_vulkanDevice->GetVkDevice().destroyImage(m_vkImage);

    m_pVulkanDeviceMemory.reset();
}

void VulkanImage::UploadImageToGPU()
{
    TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage();
    TransitionImageLayout(vk::ImageLayout::eTransferDstOptimal, m_vkImageLayout);
}

void VulkanImage::copyBufferToImage()
{
    VulkanCommandPool* cmdPool = m_vulkanDevice->GetPVulkanCmdPool();
    vk::CommandBuffer cmd =
    cmdPool->BeginSingleTimeCommand();
    {
        auto region = vk::BufferImageCopy()
                .setBufferImageHeight(0)
                .setBufferOffset(0)
                .setBufferRowLength(0)
                .setImageSubresource(vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                    0,0,1
                    })
                .setImageOffset(vk::Offset3D{0,0,0})
                .setImageExtent(vk::Extent3D{(uint32_t)m_pRawData->GetWidth(), (uint32_t)m_pRawData->GetHeight(), 1})
                ;
        cmd.copyBufferToImage(*m_pVulkanStagingBuffer->GetPVkBuf(), m_vkImage, vk::ImageLayout::eTransferDstOptimal, region);
    }
    cmdPool->EndSingleTimeCommand(cmd, m_vulkanDevice->GetVkGraphicQueue());
}

void VulkanImage::createImage()
{
    auto createInfo = vk::ImageCreateInfo()
                .setImageType(vk::ImageType::e2D)
                .setExtent(vk::Extent3D{(uint32_t)m_pRawData->GetWidth(), (uint32_t)m_pRawData->GetHeight(), 1})
                .setMipLevels(1)
                .setArrayLayers(1)
                .setFormat(m_vkFormat)
                .setTiling(m_vkImageTiling)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setUsage(m_vkImageUsageFlags)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setSharingMode(vk::SharingMode::eExclusive)
                ;
    m_vkImage = m_vulkanDevice->GetVkDevice().createImage(createInfo);
}

void VulkanImage::allocDeviceMemory(vk::MemoryPropertyFlags memProps)
{
    vk::MemoryRequirements requirements = m_vulkanDevice->GetVkDevice().getImageMemoryRequirements(m_vkImage);
    m_pVulkanDeviceMemory.reset(new VulkanDeviceMemory(m_vulkanDevice, requirements, memProps));
    m_pVulkanDeviceMemory->Bind(this);
}

void VulkanImage::createStagingBuffer()
{
    assert(m_vkImageUsageFlags & vk::ImageUsageFlagBits::eTransferDst);

    m_pVulkanStagingBuffer.reset(
        new VulkanBuffer
        (
            m_vulkanDevice, m_pRawData->GetDataSize(),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::SharingMode::eExclusive
        )
    );
    m_pVulkanStagingBuffer->FillingBufferOneTime(m_pRawData->GetData(), 0, m_pRawData->GetDataSize());
    // m_pRawData->FreeData();
}

void VulkanImage::createImageView()
{
    auto viewInfo = vk::ImageViewCreateInfo()
                .setImage(m_vkImage)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(m_vkFormat)
                .setSubresourceRange(vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0,1,0,1
                    })
                ;
    m_vkImageView = m_vulkanDevice->GetVkDevice().createImageView(viewInfo);
}

void VulkanImage::createSampler()
{
    auto deviceProps = m_vulkanDevice->GetVulkanPhysicalDevice()->GetPhysicalDeviceInfo().deviceProps;

    auto samplerInfo = vk::SamplerCreateInfo()
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setAnisotropyEnable(VK_TRUE)
                .setMaxAnisotropy(deviceProps.limits.maxSamplerAnisotropy)
                .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(VK_FALSE)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(vk::CompareOp::eAlways)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
                .setMaxLod(0.0f)
                ;
    m_vkSampler = m_vulkanDevice->GetVkDevice().createSampler(samplerInfo);
}

void VulkanImage::TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    VulkanCommandPool* cmdPool = m_vulkanDevice->GetPVulkanCmdPool();
    vk::CommandBuffer cmd =
    cmdPool->BeginSingleTimeCommand();
    {
        vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
                                .setImage(m_vkImage)
                                .setOldLayout(oldLayout)
                                .setNewLayout(newLayout)
                                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                .setSubresourceRange(vk::ImageSubresourceRange{
                                    vk::ImageAspectFlagBits::eColor,
                                    0,1,0,1
                                    })
                                ;
        vk::PipelineStageFlags srcStage, dstStage;
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits(0))
                    .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            srcStage = vk::PipelineStageFlagBits::eTransfer;
            dstStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }
        cmd.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits(0), {}, {}, {barrier});
    }
    cmdPool->EndSingleTimeCommand(cmd, m_vulkanDevice->GetVkGraphicQueue());
}
