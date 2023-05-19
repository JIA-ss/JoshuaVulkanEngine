#include "VulkanFramebuffer.h"
#include <algorithm>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

std::vector<VulkanFramebuffer::Attachment>
VulkanFramebuffer::Attachment::Merge(const std::vector<Attachment>& a, const std::vector<Attachment>& b)
{
    if (a.empty())
    {
        return b;
    }

    if (b.empty())
    {
        return a;
    }

    auto getMergedLoadOp = [](const vk::AttachmentLoadOp& aop, const vk::AttachmentLoadOp& bop,
                              vk::AttachmentLoadOp& mergeOp) -> bool
    {
        if (aop == vk::AttachmentLoadOp::eDontCare)
        {
            mergeOp = bop;
            return true;
        }
        if (bop == vk::AttachmentLoadOp::eDontCare)
        {
            mergeOp = aop;
            return true;
        }
        if (aop == bop)
        {
            mergeOp = aop;
            return true;
        }
        return false;
    };

    auto getMergedStoreOp = [](const vk::AttachmentStoreOp& aop, const vk::AttachmentStoreOp& bop,
                               vk::AttachmentStoreOp& mergeOp) -> bool
    {
        if (aop == vk::AttachmentStoreOp::eDontCare)
        {
            mergeOp = bop;
            return true;
        }
        if (bop == vk::AttachmentStoreOp::eDontCare)
        {
            mergeOp = aop;
            return true;
        }
        if (aop == bop)
        {
            mergeOp = aop;
            return true;
        }
        return false;
    };

    auto maxAttachmentSize = std::max(a.size(), b.size());
    auto minAttachmentSize = std::min(a.size(), b.size());
    std::vector<VulkanFramebuffer::Attachment> result(maxAttachmentSize);
    for (int i = 0; i < minAttachmentSize; i++)
    {
#define CHECK_AND_RETURN_IF_NOT_EQUAL(a, b)                                                                            \
    if (a != b)                                                                                                        \
    {                                                                                                                  \
        return {};                                                                                                     \
    }

        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].samples, b[i].samples);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].resourceInitialLayout, b[i].resourceInitialLayout);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].resourceFinalLayout, b[i].resourceFinalLayout);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].resourceFormat, b[i].resourceFormat);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].type, b[i].type);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].resource.vkImage, b[i].resource.vkImage);
        CHECK_AND_RETURN_IF_NOT_EQUAL(a[i].resource.vkImageView, b[i].resource.vkImageView);
#undef CHECK_AND_RETURN_IF_NOT_EQUAL

        result[i] = a[i];

        if (!getMergedLoadOp(a[i].loadOp, b[i].loadOp, result[i].loadOp))
        {
            return {};
        }
        if (!getMergedLoadOp(a[i].stencilLoadOp, b[i].stencilLoadOp, result[i].stencilLoadOp))
        {
            return {};
        }
        if (!getMergedStoreOp(a[i].storeOp, b[i].storeOp, result[i].storeOp))
        {
            return {};
        }
        if (!getMergedStoreOp(a[i].stencilStoreOp, b[i].stencilStoreOp, result[i].stencilStoreOp))
        {
            return {};
        }
    }

    return result;
}

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
        .setFormat(resourceFormat);
}

vk::AttachmentReference VulkanFramebuffer::Attachment::GetVkAttachmentReference(uint32_t attachmentId) const
{
    return vk::AttachmentReference{attachmentId, attachmentReferenceLayout};
}

VulkanFramebuffer::VulkanFramebuffer(
    VulkanDevice* device, VulkanRenderPass* renderpass, uint32_t width, uint32_t height, uint32_t layer,
    const std::vector<Attachment>& attachments)
    : m_pDevice(device), m_pRenderpass(renderpass), m_width(width), m_height(height), m_layer(layer),
      m_attachments(attachments)
{
    ZoneScopedN("VulkanFramebuffer::VulkanFramebuffer");
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
                            .setRenderPass(m_pRenderpass->GetVkRenderPass());

    m_vkFramebuffer = m_pDevice->GetVkDevice().createFramebuffer(fbCreateInfo);
}

VulkanFramebuffer::~VulkanFramebuffer()
{
    ZoneScopedN("VulkanFramebuffer::~VulkanFramebuffer");
    m_attachments.clear();
    if (m_vkFramebuffer)
    {
        m_pDevice->GetVkDevice().destroyFramebuffer(m_vkFramebuffer);
        m_vkFramebuffer = nullptr;
    }
}
