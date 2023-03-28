#pragma once
#include "PostPass.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"

namespace Render {

class PresetPostPass : public PostPass
{
public:
    explicit PresetPostPass(
        RHI::VulkanDevice* device,
        Camera* cam,
        Lights* light,
        uint32_t fbWidth, uint32_t fbHeight,
        std::shared_ptr<RHI::VulkanShaderSet> shaderSet,
        const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments
        );
    ~PresetPostPass() override = default;
    void Prepare() override;
    void Render(vk::CommandBuffer& cmdBuffer, std::vector<vk::DescriptorSet>& tobinding, vk::Framebuffer fb) override;
    RHI::VulkanDescriptorSets* GetDescriptorSets() const override { return nullptr; }
protected:
    // 1. layout
    void prepareLayout() override;
    // 2. attachments
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    // 3. renderpass
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    // 4. framebuffer
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) override { }
    // 5. pipeline
    void preparePipeline() override;
    // 6. descriptor sets
    void prepareOutputDescriptorSets() override { }
    void preparePlaneModel(Camera* cam, Lights* light);
protected:
    Camera* m_pCamera;
    Lights* m_pLights;
    std::vector<RHI::VulkanFramebuffer::Attachment> m_attachments;
    std::shared_ptr<RHI::VulkanShaderSet> m_pShaderSet;
    std::unique_ptr<RHI::Model> m_pPlaneModel;
};

}