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
{
    ZoneScopedN("VulkanImageResource::VulkanImageResource");
    m_native.config = config;
    createImage();

    vk::MemoryRequirements requirements = m_vulkanDevice->GetVkDevice().getImageMemoryRequirements(m_native.vkImage.value());
    m_pVulkanDeviceMemory.reset(new VulkanDeviceMemory(m_vulkanDevice, requirements, memProps));
    m_pVulkanDeviceMemory->Bind(this);

    createImageView();
}

VulkanImageResource::~VulkanImageResource()
{
    ZoneScopedN("VulkanImageResource::~VulkanImageResource");
    if (m_native.vkImageView)
    {
        m_vulkanDevice->GetVkDevice().destroyImageView(m_native.vkImageView.value());
    }
    if (m_native.vkImage)
    {
        m_vulkanDevice->GetVkDevice().destroyImage(m_native.vkImage.value());
    }
    m_pVulkanDeviceMemory.reset();
}

void VulkanImageResource::TransitionImageLayout(
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage
)
{
    ZoneScopedN("VulkanImageResource::TransitionImageLayout");
    VulkanCommandPool* cmdPool = m_vulkanDevice->GetPVulkanCmdPool();
    vk::CommandBuffer cmd =
    cmdPool->BeginSingleTimeCommand();
    {
        TransitionImageLayout(cmd, oldLayout, newLayout, srcStage, dstStage);
    }
    cmdPool->EndSingleTimeCommand(cmd, m_vulkanDevice->GetVkGraphicQueue());
}

void VulkanImageResource::TransitionImageLayout(
        vk::CommandBuffer cmd,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage,
        vk::PipelineStageFlags dstStage
)
{
    ZoneScopedN("VulkanImageResource::TransitionImageLayout");
    vk::ImageMemoryBarrier imageMemoryBarrier = vk::ImageMemoryBarrier()
                            .setImage(m_native.vkImage.value())
                            .setOldLayout(oldLayout)
                            .setNewLayout(newLayout)
                            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                            .setSubresourceRange(m_native.config.value().subresourceRange)
                            ;
    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldLayout)
    {
    case vk::ImageLayout::eUndefined:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlags(0));
        break;

    case vk::ImageLayout::ePreinitialized:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite);
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        break;

    case vk::ImageLayout::eTransferDstOptimal:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        break;
    default:
        // Other source layouts aren't handled (yet)
        assert(false);
        break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        break;

    case vk::ImageLayout::eTransferSrcOptimal:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        break;

    case vk::ImageLayout::eColorAttachmentOptimal:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        break;

    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite | imageMemoryBarrier.dstAccessMask);
        break;

    case vk::ImageLayout::eShaderReadOnlyOptimal:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == vk::AccessFlags(0))
        {
            imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
        }
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        break;
    case vk::ImageLayout::eGeneral:
        imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        break;
    default:
        // Other source layouts aren't handled (yet)
        assert(false);
        break;
    }

    cmd.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits(0), {}, {}, {imageMemoryBarrier});
}

void VulkanImageResource::CopyTo(vk::CommandBuffer cmd, VulkanImageResource* target, vk::ImageCopy copyRegion)
{
    ZoneScopedN("VulkanImageResource::CopyTo");
    cmd.copyImage(m_native.vkImage.value(), vk::ImageLayout::eTransferSrcOptimal, target->m_native.vkImage.value(), vk::ImageLayout::eTransferDstOptimal, copyRegion);
}

void VulkanImageResource::createImage()
{
    ZoneScopedN("VulkanImageResource::createImage");
    auto createInfo = vk::ImageCreateInfo()
                .setImageType(m_native.config.value().imageType)
                .setExtent(m_native.config.value().extent)
                .setMipLevels(m_native.config.value().miplevel)
                .setArrayLayers(m_native.config.value().arrayLayer)
                .setFormat(m_native.config.value().format)
                .setTiling(m_native.config.value().imageTiling)
                .setInitialLayout(m_native.config.value().initialLayout)
                .setUsage(m_native.config.value().imageUsage)
                .setSamples(m_native.config.value().sampleCount)
                .setSharingMode(m_native.config.value().sharingMode)
                .setFlags(m_native.config.value().flags)
                ;
    m_native.vkImage = m_vulkanDevice->GetVkDevice().createImage(createInfo);
}

void VulkanImageResource::createImageView()
{
    ZoneScopedN("VulkanImageResource::createImageView");
    auto viewInfo = vk::ImageViewCreateInfo()
                .setImage(m_native.vkImage.value())
                .setViewType(m_native.config.value().imageViewType)
                .setFormat(m_native.config.value().format)
                .setSubresourceRange(m_native.config.value().subresourceRange)
                ;
    m_native.vkImageView = m_vulkanDevice->GetVkDevice().createImageView(viewInfo);
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
    ZoneScopedN("VulkanImageSampler::VulkanImageSampler");
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
    ZoneScopedN("VulkanImageSampler::~VulkanImageSampler");
    m_pVulkanStagingBuffer.reset();
    m_pRawData = nullptr;

    m_vulkanDevice->GetVkDevice().destroySampler(m_vkSampler);
    m_pVulkanImageResource.reset();
}

void VulkanImageSampler::UploadImageToGPU()
{
    ZoneScopedN("VulkanImageSampler::UploadImageToGPU");
    m_pVulkanImageResource->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage();
    m_pVulkanImageResource->TransitionImageLayout(vk::ImageLayout::eTransferDstOptimal, m_config.imageLayout);
}

void VulkanImageSampler::copyBufferToImage()
{
    ZoneScopedN("VulkanImageSampler::copyBufferToImage");
    VulkanCommandPool* cmdPool = m_vulkanDevice->GetPVulkanCmdPool();
    vk::CommandBuffer cmd =
    cmdPool->BeginSingleTimeCommand();
    {
        ZoneScopedN("VulkanImageSampler::copyBufferToImage:: cmd recording");
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
    ZoneScopedN("VulkanImageSampler::createStagingBuffer");
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
    ZoneScopedN("VulkanImageSampler::createSampler");
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
    ZoneScopedN("VulkanImageSampler::ConvertDevice");
    std::shared_ptr<VulkanImageSampler> sampler(
        new VulkanImageSampler(device, m_pRawData, m_memProps, m_config, m_pVulkanImageResource->m_native.config.value())
    );
    return sampler;
}