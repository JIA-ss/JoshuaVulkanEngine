#pragma once
#include <stdint.h>
#include <vulkan/vulkan.hpp>


#include "Runtime/VulkanRHI/VulkanRHI.h"

RHI_NAMESPACE_BEGIN
class VulkanDevice;
class VulkanRenderPass;
class VulkanImageResource;
class VulkanFramebuffer
{
public:
private:
public:
    VulkanFramebuffer(VulkanDevice* device,
        VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer = 1,
        const std::vector<VulkanImageResource*>& attachments = {}
    );
    ~VulkanFramebuffer();

    inline vk::Framebuffer GetVkFramebuffer() { return m_vkFramebuffer; }
    inline std::vector<VulkanImageResource*> GetAttachments() { return m_attachments; }
    inline uint32_t GetWidth() { return m_width; }
    inline uint32_t GetHeight() { return m_height; }
    inline uint32_t GetLayer() { return m_layer; }
private:
    VulkanDevice* m_pDevice;
    VulkanRenderPass* m_pRenderpass;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_layer;
    std::vector<VulkanImageResource*> m_attachments;

    vk::Framebuffer m_vkFramebuffer;
};
RHI_NAMESPACE_END