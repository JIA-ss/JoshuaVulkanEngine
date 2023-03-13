#include "VulkanFramebuffer.h"
#include <algorithm>
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice* device,
    VulkanRenderPass* renderpass,
    uint32_t width, uint32_t height, uint32_t layer,
    const std::vector<VulkanImageResource*>& attachments)
    : m_pDevice(device)
    , m_pRenderpass(renderpass)
    , m_width(width)
    , m_height(height)
    , m_layer(layer)
    , m_attachments(attachments)
{
    std::vector<vk::ImageView> att;
    att.resize(m_attachments.size());
    for (int i = 0; i < m_attachments.size(); i++)
    {
        att[i] = m_attachments[i]->GetVkImageView();
    }
    auto fbCreateInfo = vk::FramebufferCreateInfo()
            .setAttachments(att)
            .setWidth(m_width)
            .setHeight(m_height)
            .setLayers(m_layer)
            .setRenderPass(m_pRenderpass->GetVkRenderPass())
            ;

    m_vkFramebuffer = m_pDevice->GetVkDevice().createFramebuffer(fbCreateInfo);
}


VulkanFramebuffer::~VulkanFramebuffer()
{
    if (m_vkFramebuffer)
    {
        m_pDevice->GetVkDevice().destroyFramebuffer(m_vkFramebuffer);
        m_vkFramebuffer = nullptr;
    }
}