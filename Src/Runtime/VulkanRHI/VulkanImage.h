#pragma once
#include "Runtime/VulkanRHI/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanImage
{
public:
private:
    VulkanDevice* m_vulkanDevice;

    std::unique_ptr<VulkanBuffer> m_pVulkanStagingBuffer;
    std::shared_ptr<util::texture::RawData> m_pRawData;
    std::unique_ptr<VulkanDeviceMemory> m_pVulkanDeviceMemory;

    vk::Image m_vkImage;
    vk::ImageView m_vkImageView;
    vk::Sampler m_vkSampler;

    vk::Format m_vkFormat;
    vk::ImageTiling m_vkImageTiling;
    vk::ImageUsageFlags m_vkImageUsageFlags;
    vk::ImageLayout m_vkImageLayout;
public:
    explicit VulkanImage(
        VulkanDevice* device,
        std::shared_ptr<util::texture::RawData> rawData,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags memProps,
        vk::ImageLayout layout
    );
    ~VulkanImage();

    void TransitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    inline vk::Image* GetPVkImage() { return &m_vkImage; }
    inline vk::ImageView* GetPVkImageView() { return &m_vkImageView; }
    inline vk::Sampler* GetPVkSampler() { return &m_vkSampler; }

    void UploadImageToGPU();
private:
    void createImage();
    void allocDeviceMemory(vk::MemoryPropertyFlags memProps);
    void createStagingBuffer();
    void createImageView();
    void createSampler();
    void copyBufferToImage();
};
RHI_NAMESPACE_END