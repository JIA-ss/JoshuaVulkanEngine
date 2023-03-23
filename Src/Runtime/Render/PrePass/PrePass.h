#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
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

    void Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models);
    RHI::VulkanDescriptorSets* GetDescriptorSets() const { return m_pDescriptors.get(); }
private:
    void prepareLayout();
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments);
    void preparePipeline();
    void prepareOutputDescriptorSets();
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
    std::shared_ptr<RHI::VulkanPipelineLayout> m_pipelineLayout;
    std::unique_ptr<RHI::VulkanFramebuffer> m_framebuffers;
    std::unique_ptr<RHI::VulkanRenderPass> m_pRenderPass;
    AttachmentResources m_attachmentResources;


    std::unique_ptr<RHI::VulkanFramebuffer> m_pFramebuffer;
    std::unique_ptr<RHI::VulkanDescriptorPool> m_pDescriptorPool;
    std::shared_ptr<RHI::VulkanDescriptorSets> m_pDescriptors;
};

}