#pragma once

#include "PrePass.h"

namespace Render
{

class ZPrePass : public PrePass
{
public:
    explicit ZPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight);
    ~ZPrePass() override;

    void Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models) override { assert(false); }
    RHI::VulkanDescriptorSets* GetDescriptorSets() const override { return m_pDescriptors.get(); }
private:
    void prepareLayout() override;
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) override;
    void preparePipeline() override;
    void prepareOutputDescriptorSets() override;
private:
    std::unique_ptr<RHI::VulkanImageSampler> m_pDepthImageSampler;
    uint32_t m_fbWidth;
    uint32_t m_fbHeight;
};

}