#include "VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING


VulkanImageResource::Config VulkanImageResource::Config::CubeMap(uint32_t width, uint32_t height, uint32_t miplevels, uint32_t faceCount/* = 6 */)
{
    Config config;
    config.extent = vk::Extent3D{width, height, 1};
    config.arrayLayer = faceCount;
    config.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    config.miplevel = miplevels;
    config.subresourceRange.setBaseMipLevel(0)
                            .setLevelCount(miplevels)
                            .setLayerCount(faceCount);

    config.imageViewType = vk::ImageViewType::eCube;
    return config;
}

VulkanImageSampler::Config VulkanImageSampler::Config::CubeMap(uint32_t miplevel)
{
    VulkanImageSampler::Config config;
    config.uAddressMode = vk::SamplerAddressMode::eClampToEdge;
    config.compareOp = vk::CompareOp::eNever;
    config.maxLod = miplevel;
    config.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    return config;
}

VulkanImageResource::VulkanImageResource(
    VulkanDevice* device,
    vk::MemoryPropertyFlags memProps,
    Config config
)
    : m_vulkanDevice(device)
    , m_config(config)
{
    createImage();

    vk::MemoryRequirements requirements = m_vulkanDevice->GetVkDevice().getImageMemoryRequirements(m_vkImage);
    m_pVulkanDeviceMemory.reset(new VulkanDeviceMemory(m_vulkanDevice, requirements, memProps));
    m_pVulkanDeviceMemory->Bind(this);

    createImageView();
}

VulkanImageResource::~VulkanImageResource()
{
    m_vulkanDevice->GetVkDevice().destroyImageView(m_vkImageView);
    m_vulkanDevice->GetVkDevice().destroyImage(m_vkImage);
    m_pVulkanDeviceMemory.reset();
}

void VulkanImageResource::TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
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
                                .setSubresourceRange(m_config.subresourceRange)
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
        else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits(0))
                    .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    ;
            srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            srcStage = vk::PipelineStageFlagBits::eLateFragmentTests;
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

void VulkanImageResource::createImage()
{
    auto createInfo = vk::ImageCreateInfo()
                .setImageType(m_config.imageType)
                .setExtent(m_config.extent)
                .setMipLevels(m_config.miplevel)
                .setArrayLayers(m_config.arrayLayer)
                .setFormat(m_config.format)
                .setTiling(m_config.imageTiling)
                .setInitialLayout(m_config.initialLayout)
                .setUsage(m_config.imageUsage)
                .setSamples(m_config.sampleCount)
                .setSharingMode(m_config.sharingMode)
                .setFlags(m_config.flags)
                ;
    m_vkImage = m_vulkanDevice->GetVkDevice().createImage(createInfo);
}

void VulkanImageResource::createImageView()
{
    auto viewInfo = vk::ImageViewCreateInfo()
                .setImage(m_vkImage)
                .setViewType(m_config.imageViewType)
                .setFormat(m_config.format)
                .setSubresourceRange(m_config.subresourceRange)
                ;
    m_vkImageView = m_vulkanDevice->GetVkDevice().createImageView(viewInfo);
}

VulkanImageSampler::VulkanImageSampler(
        VulkanDevice* device,
        std::shared_ptr<Util::Texture::RawData> rawData,
        vk::MemoryPropertyFlags memProps,
        Config config,
        VulkanImageResource::Config resourceConfig
)
    : m_vulkanDevice(device)
    , m_pRawData(rawData)
    , m_config(config)
    , m_memProps(memProps)
{

    m_pVulkanImageResource.reset(new VulkanImageResource(device, memProps, resourceConfig));

    createSampler();

    if (m_pRawData)
    {
        createStagingBuffer();
        UploadImageToGPU();
    }
}

VulkanImageSampler::~VulkanImageSampler()
{
    m_pVulkanStagingBuffer.reset();
    m_pRawData = nullptr;

    m_vulkanDevice->GetVkDevice().destroySampler(m_vkSampler);
    m_pVulkanImageResource.reset();
}

void VulkanImageSampler::UploadImageToGPU()
{
    m_pVulkanImageResource->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage();
    m_pVulkanImageResource->TransitionImageLayout(vk::ImageLayout::eTransferDstOptimal, m_config.imageLayout);
}

void VulkanImageSampler::copyBufferToImage()
{
    VulkanCommandPool* cmdPool = m_vulkanDevice->GetPVulkanCmdPool();
    vk::CommandBuffer cmd =
    cmdPool->BeginSingleTimeCommand();
    {
        std::vector<vk::BufferImageCopy> regions;
        for (uint32_t face = 0; face < m_pVulkanImageResource->GetConfig().arrayLayer; face++)
        {
            for (uint32_t level = 0; level < m_pVulkanImageResource->GetConfig().miplevel; level++)
            {
                size_t offset = m_pRawData->GetLevelOffset(level, face);
                uint32_t width = (uint32_t)m_pRawData->GetWidth() >> level;
                uint32_t height = (uint32_t)m_pRawData->GetHeight() >> level;
                auto region = vk::BufferImageCopy()
                        .setBufferOffset(offset)
                        .setImageExtent(vk::Extent3D{width, height, 1})
                        .setImageSubresource(vk::ImageSubresourceLayers{
                            vk::ImageAspectFlagBits::eColor,
                            level,face,1
                            })
                        .setBufferImageHeight(0)
                        .setBufferRowLength(0)
                        .setImageOffset(vk::Offset3D{0,0,0})
                        ;
                regions.emplace_back(region);
            }
        }
        cmd.copyBufferToImage(*m_pVulkanStagingBuffer->GetPVkBuf(), m_pVulkanImageResource->GetVkImage(), vk::ImageLayout::eTransferDstOptimal, regions);
    }
    cmdPool->EndSingleTimeCommand(cmd, m_vulkanDevice->GetVkGraphicQueue());
}

void VulkanImageSampler::createStagingBuffer()
{
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

void VulkanImageSampler::createSampler()
{
    auto deviceProps = m_vulkanDevice->GetVulkanPhysicalDevice()->GetPhysicalDeviceInfo().deviceProps;

    auto samplerInfo = vk::SamplerCreateInfo()
                .setMagFilter(m_config.magFilter)
                .setMinFilter(m_config.minFilter)
                .setAddressModeU(m_config.uAddressMode)
                .setAddressModeV(m_config.vAddressMode)
                .setAddressModeW(m_config.wAddressMode)
                .setAnisotropyEnable(m_config.anisotropyEnable)
                .setMaxAnisotropy(deviceProps.limits.maxSamplerAnisotropy)
                .setBorderColor(m_config.borderColor)
                .setUnnormalizedCoordinates(m_config.unnormalizedCoordinates)
                .setCompareEnable(m_config.compareEnable)
                .setCompareOp(m_config.compareOp)
                .setMipmapMode(m_config.mipmapMode)
                .setMipLodBias(m_config.mipLodBias)
                .setMinLod(m_config.minLod)
                .setMaxLod(m_config.maxLod)
                ;
    m_vkSampler = m_vulkanDevice->GetVkDevice().createSampler(samplerInfo);
}


std::shared_ptr<VulkanImageSampler> VulkanImageSampler::ConvertDevice(VulkanDevice* device)
{
    std::shared_ptr<VulkanImageSampler> sampler(
        new VulkanImageSampler(device, m_pRawData, m_memProps, m_config, m_pVulkanImageResource->m_config)
    );
    return sampler;
}