#include "VulkanFramebuffer.h"
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

vk::AttachmentDescription VulkanFramebuffer::Attachment::GetVkAttachmentDescription() const
{
    return vk::AttachmentDescription()
            .setSamples(samples)
            .setLoadOp(loadOp)
            .setStoreOp(storeOp)
            .setStencilLoadOp(stencilLoadOp)
            .setStencilStoreOp(stencilStoreOp)
            .setInitialLayout(resourceInitialLayout)
            .setFinalLayout(resourceFinalLayout)
            .setFormat(resourceFormat)
            ;
}

vk::AttachmentReference VulkanFramebuffer::Attachment::GetVkAttachmentReference(uint32_t attachmentId) const
{
    return vk::AttachmentReference{attachmentId, attachmentLayout};
}

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice* device,
    VulkanRenderPass* renderpass,
    uint32_t width, uint32_t height, uint32_t layer,
    const std::vector<Attachment>& attachments)
    : m_pDevice(device)
    , m_pRenderpass(renderpass)
    , m_width(width)
    , m_height(height)
    , m_layer(layer)
    , m_attachments(attachments)
{
    std::vector<vk::ImageView> vkAtt;
    vkAtt.resize(m_attachments.size());
    for (int i = 0; i < m_attachments.size(); i++)
    {
        vkAtt[i] = m_attachments[i].resource.vkImageView.value();
    }
    auto fbCreateInfo = vk::FramebufferCreateInfo()
            .setAttachments(vkAtt)
            .setWidth(m_width)
            .setHeight(m_height)
            .setLayers(m_layer)
            .setRenderPass(m_pRenderpass->GetVkRenderPass())
            ;

    m_vkFramebuffer = m_pDevice->GetVkDevice().createFramebuffer(fbCreateInfo);
}


VulkanFramebuffer::~VulkanFramebuffer()
{
    m_attachments.clear();
    if (m_vkFramebuffer)
    {
        m_pDevice->GetVkDevice().destroyFramebuffer(m_vkFramebuffer);
        m_vkFramebuffer = nullptr;
    }
}
