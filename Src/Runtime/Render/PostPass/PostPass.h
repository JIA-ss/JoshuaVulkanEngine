#pragma once
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class PostPass
{
public:
    explicit PostPass(RHI::VulkanDevice* device, uint32_t fbWidth, uint32_t fbHeight);
    virtual ~PostPass() = default;
    virtual void Prepare();
    virtual void Render(vk::CommandBuffer& cmdBuffer, std::vector<vk::DescriptorSet>& tobinding, vk::Framebuffer fb) = 0;
    virtual RHI::VulkanDescriptorSets* GetDescriptorSets() const = 0;
    RHI::VulkanRenderPass* GetPVulkanRenderPass() { return m_pRenderPass.get(); }
protected:
    // 1. layout
    virtual void prepareLayout();
    // 2. attachments
    virtual void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) = 0;
    // 3. renderpass
    virtual void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) = 0;
    // 4. framebuffer
    virtual void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) = 0;
    // 5. pipeline
    virtual void preparePipeline() = 0;
    // 6. descriptor sets
    virtual void prepareOutputDescriptorSets() = 0;
protected:
    RHI::VulkanDevice* m_pDevice;
    uint32_t m_width;
    uint32_t m_height;
    std::shared_ptr<RHI::VulkanPipelineLayout> m_pPipelineLayout;
    std::unique_ptr<RHI::VulkanRenderPass> m_pRenderPass;
    std::unique_ptr<RHI::VulkanFramebuffer> m_pFramebuffer;
    std::unique_ptr<RHI::VulkanDescriptorPool> m_pDescriptorPool;
    std::shared_ptr<RHI::VulkanDescriptorSets> m_pDescriptors;
};

}