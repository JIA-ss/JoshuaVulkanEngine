#include "PresentPostPass.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/Render/PostPass/PostPass.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "vulkan/vulkan_enums.hpp"

using namespace Render;

PresetPostPass::PresetPostPass(
    RHI::VulkanDevice* device,
    Camera* cam,
    Lights* light,
    uint32_t fbWidth, uint32_t fbHeight,
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet,
    const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments
)
    : PostPass(device, fbWidth, fbHeight)
    , m_attachments(attachments)
    , m_pShaderSet(shaderSet)
    , m_pCamera(cam)
    , m_pLights(light)
{

}

void PresetPostPass::Prepare()
{
    prepareLayout();
    {
        std::vector<RHI::VulkanFramebuffer::Attachment> attachments;
        prepareAttachments(attachments);
        prepareRenderPass(attachments);
        prepareFramebuffer(std::move(attachments));
    }
    preparePipeline();
    prepareOutputDescriptorSets();
    preparePlaneModel(m_pCamera, m_pLights);
}

void PresetPostPass::Render(vk::CommandBuffer& cmdBuffer, std::vector<vk::DescriptorSet>& tobinding, vk::Framebuffer fb)
{
    ZoneScopedN("PresetPostPass::Render");
    std::vector<vk::ClearValue> clears(2);
    clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
    clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
    if (m_pDevice->GetVulkanPhysicalDevice()->IsUsingMSAA())
    {
        clears.push_back(clears[0]);
    }

    m_pRenderPass->Begin(cmdBuffer, clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, fb);
    {
        auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
        vk::Rect2D rect{{0,0},extent};
        cmdBuffer.setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
        cmdBuffer.setScissor(0,rect);

        // m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "skybox");
        // m_pSkyboxModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);

        m_pRenderPass->BindGraphicPipeline(cmdBuffer, "present");
        // m_vkCmds[m_frameIdxInFlight].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pPipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
        m_pPlaneModel->DrawWithNoMaterial(cmdBuffer, m_pPipelineLayout.get(), tobinding);
    }
    m_pRenderPass->End(cmdBuffer);
}
void PresetPostPass::prepareLayout()
{
    PostPass::prepareLayout();
}

void PresetPostPass::prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    attachments = m_attachments;
}

void PresetPostPass::prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice)
                        .SetAttachments(attachments)
                        .SetDefaultSubpass()
                        .AddSubpassDependency(
                            vk::SubpassDependency()
                                    .setSrcSubpass(0)
                                    .setDstSubpass(VK_SUBPASS_EXTERNAL)
                                    .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                                    .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                        )
                        .buildUnique();
}

void PresetPostPass::preparePipeline()
{
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice, m_pRenderPass.get())
                        .SetVulkanPipelineLayout(m_pPipelineLayout)
                        .SetshaderSet(m_pShaderSet)
                        .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("present", std::move(pipeline));
}

void PresetPostPass::preparePlaneModel(Camera* cam, Lights* light)
{
    m_pPlaneModel = RHI::ModelPresets::CreatePlaneModel(m_pDevice, m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER.get());
    auto camUboInfo = cam->GetUboInfo();
    auto lightUboInfo = light->GetUboInfo();
    m_pPlaneModel->InitUniformDescriptorSets({camUboInfo, lightUboInfo}, m_pDevice->GetDescLayoutPresets().UBO.get());
}