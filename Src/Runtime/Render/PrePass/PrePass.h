#pragma once
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class PrePass
{
public:
    explicit PrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight);
    ~PrePass();
private:
    void prepareLayout();
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments);
    void preparePipeline();
private:
    struct AttachmentResources
    {
        std::unique_ptr<RHI::VulkanImageSampler> position;
        std::unique_ptr<RHI::VulkanImageSampler> normal;
        std::unique_ptr<RHI::VulkanImageSampler> albedo;
        std::unique_ptr<RHI::VulkanImageSampler> depth;
    };
private:
    RHI::VulkanDevice* m_pDevice;
    Camera* m_pCamera;
    uint32_t m_fbWidth;
    uint32_t m_fbHeight;
    std::array<std::unique_ptr<RHI::VulkanFramebuffer>, MAX_FRAMES_IN_FLIGHT> m_framebuffers;
    std::unique_ptr<RHI::VulkanRenderPass> m_pRenderPass;
    AttachmentResources m_attachmentResources;



    std::unique_ptr<RHI::VulkanFramebuffer> m_pFramebuffer;
};

}