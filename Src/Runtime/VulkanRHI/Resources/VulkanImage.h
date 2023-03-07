#pragma once
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanImageSampler;
class VulkanImageResource
{
    friend class VulkanImageSampler;
public:
    struct Config
    {
        vk::Extent3D                extent;
        // vk::ImageSubresourceLayers  subresourceLayers = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1};;
        vk::ImageSubresourceRange   subresourceRange = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        vk::ImageUsageFlags         imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
        vk::ImageType               imageType = vk::ImageType::e2D;
        vk::ImageViewType           imageViewType = vk::ImageViewType::e2D;
        vk::ImageLayout             initialLayout = vk::ImageLayout::eUndefined;
        vk::SampleCountFlagBits     sampleCount = vk::SampleCountFlagBits::e1;
        vk::Format                  format = vk::Format::eR8G8B8A8Unorm;
        vk::ImageTiling             imageTiling = vk::ImageTiling::eOptimal;
        vk::SharingMode             sharingMode = vk::SharingMode::eExclusive;
        vk::ImageCreateFlags        flags = {};
        uint32_t                    miplevel = 1;
        uint32_t                    arrayLayer = 1;

        static Config CubeMap(uint32_t width, uint32_t height, uint32_t miplevels, uint32_t faceCount = 6);
    };
protected:
    VulkanDevice* m_vulkanDevice;
    std::unique_ptr<VulkanDeviceMemory> m_pVulkanDeviceMemory;
    vk::Image m_vkImage;
    vk::ImageView m_vkImageView;

    Config m_config;
public:
    explicit VulkanImageResource(
        VulkanDevice* device,
        vk::MemoryPropertyFlags memProps,
        Config config
    );

    ~VulkanImageResource();
    Config GetConfig() { return m_config; }
public:
    void TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

public:
    inline vk::Image& GetVkImage() { return m_vkImage; }
    inline vk::ImageView& GetVkImageView() { return m_vkImageView; }

private:
    void createImage();
    void createImageView();
};

class VulkanImageSampler
{
public:
    struct Config
    {
        vk::ImageLayout         imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        vk::Filter              magFilter = vk::Filter::eLinear;
        vk::Filter              minFilter = vk::Filter::eLinear;
        vk::SamplerAddressMode  uAddressMode = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode  vAddressMode = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode  wAddressMode = vk::SamplerAddressMode::eRepeat;
        vk::Bool32              anisotropyEnable = VK_TRUE;
        vk::BorderColor         borderColor = vk::BorderColor::eIntOpaqueBlack;
        vk::Bool32              unnormalizedCoordinates = VK_FALSE;
        vk::Bool32              compareEnable = VK_FALSE;
        vk::CompareOp           compareOp = vk::CompareOp::eAlways;
        vk::SamplerMipmapMode   mipmapMode = vk::SamplerMipmapMode::eLinear;
        float                   mipLodBias = 0.0f;
        float                   minLod = 0.0f;
        float                   maxLod = 0.0f;

        static Config CubeMap(uint32_t miplevel);
    };
protected:
    VulkanDevice* m_vulkanDevice;

    std::unique_ptr<VulkanImageResource> m_pVulkanImageResource;
    std::unique_ptr<VulkanBuffer> m_pVulkanStagingBuffer;
    std::shared_ptr<Util::Texture::RawData> m_pRawData;
    Config m_config;

    vk::Sampler m_vkSampler;
    vk::MemoryPropertyFlags m_memProps;
public:
    explicit VulkanImageSampler(
        VulkanDevice* device,
        std::shared_ptr<Util::Texture::RawData> rawData,
        vk::MemoryPropertyFlags memProps,
        Config config,
        VulkanImageResource::Config resourceConfig
    );
    ~VulkanImageSampler();
    inline vk::ImageLayout GetImageLayout() { return m_config.imageLayout; }
    inline vk::Image* GetPVkImage() { return &m_pVulkanImageResource->GetVkImage(); }
    inline vk::ImageView* GetPVkImageView() { return &m_pVulkanImageResource->GetVkImageView(); }
    inline vk::Sampler* GetPVkSampler() { return m_vkSampler ? &m_vkSampler : nullptr; }
    inline VulkanImageResource* GetPImageResource() { return m_pVulkanImageResource.get(); }
    void UploadImageToGPU();
    Config GetConfig() { return m_config; }
    std::shared_ptr<VulkanImageSampler> ConvertDevice(VulkanDevice* device);
private:

    void createStagingBuffer();
    void createSampler();
    void copyBufferToImage();
};

RHI_NAMESPACE_END