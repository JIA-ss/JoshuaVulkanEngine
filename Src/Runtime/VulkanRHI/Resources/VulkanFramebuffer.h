#pragma once
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_BEGIN
class VulkanDevice;
class VulkanRenderPass;
class VulkanFramebuffer
{
public:
    enum AttachmentType
    {
        kUndefined,
        kDepthStencil,
        kColor,
        kResolve,
    };
    struct Attachment
    {
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
        vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
        vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
        vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        vk::ImageLayout resourceInitialLayout = vk::ImageLayout::eUndefined;
        vk::ImageLayout resourceFinalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        vk::ImageLayout attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;

        AttachmentType type = AttachmentType::kUndefined;
        vk::Format resourceFormat = vk::Format::eR8G8B8A8Unorm;
        VulkanImageResource::Native resource;
        vk::AttachmentDescription GetVkAttachmentDescription() const;
        vk::AttachmentReference GetVkAttachmentReference(uint32_t attachmentId) const;
    };
public:
    VulkanFramebuffer(VulkanDevice* device,
        VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer = 1,
        const std::vector<Attachment>& attachments = {}
    );
    ~VulkanFramebuffer();

    inline vk::Framebuffer GetVkFramebuffer() { return m_vkFramebuffer; }
    inline uint32_t GetWidth() { return m_width; }
    inline uint32_t GetHeight() { return m_height; }
    inline uint32_t GetLayer() { return m_layer; }
    inline Attachment* GetPAttachment(int id) { return &m_attachments[id]; }
private:
    VulkanDevice* m_pDevice;
    VulkanRenderPass* m_pRenderpass;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_layer;

    std::vector<Attachment> m_attachments;

    vk::Framebuffer m_vkFramebuffer;
};
RHI_NAMESPACE_END