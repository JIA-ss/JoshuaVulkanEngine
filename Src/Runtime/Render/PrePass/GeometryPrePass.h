#pragma once
#include "PrePass.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"

namespace Render {

class GeometryPrePass : public PrePass
{
public:
    explicit GeometryPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight);
    ~GeometryPrePass() override;

    void Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models) override;
    RHI::VulkanDescriptorSets* GetDescriptorSets() const override { return m_pDescriptors.get(); }
private:
    // void prepareLayout() = default;
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) override;
    void preparePipeline() override;
    void prepareOutputDescriptorSets() override;
private:
    struct AttachmentResources
    {
        std::unique_ptr<RHI::VulkanImageSampler> position;
        std::unique_ptr<RHI::VulkanImageSampler> normal;
        std::unique_ptr<RHI::VulkanImageSampler> albedo;
        std::unique_ptr<RHI::VulkanImageSampler> depth;
    };
    AttachmentResources m_attachmentResources;
    uint32_t m_fbWidth;
    uint32_t m_fbHeight;
};

}