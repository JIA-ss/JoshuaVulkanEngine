#include "MultiPipelineRenderer.h"
#include "Runtime/Render/SimpleModel/SimpleModelRenderer.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanViewportState.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
using namespace Render;

MultiPipelineRenderer::MultiPipelineRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : SimpleModelRenderer(instanceConfig, physicalConfig)
{

}

MultiPipelineRenderer::~MultiPipelineRenderer()
{
    m_pDevice->GetVkDevice().waitIdle();

    m_Pipelines.clear();
}

void MultiPipelineRenderer::prepare()
{
    SimpleModelRenderer::prepareLayout();
    preparePresentFramebufferAttachments();
    prepareRenderpass();
    preparePresentFramebuffer();
    prepareMultiPipelines();

    prepareCamera();
    prepareLight();
    prepareInputCallback();
    prepareModel();
}

void MultiPipelineRenderer::render()
{
    // wait for fence
    if (
        m_pDevice->GetVkDevice().waitForFences(m_vkFences[m_frameIdxInFlight], true, std::numeric_limits<uint64_t>::max())
        !=
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("wait for inflight fence failed");
    }

    // acquire image
    vk::Result acquireImageResult;
    try
    {
        auto res = m_pDevice->GetVkDevice().acquireNextImageKHR(m_pDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliables[m_frameIdxInFlight], nullptr);
        acquireImageResult = res.result;
        m_imageIdx = res.value;
        if (res.result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapchain();
            return;
        }
    }
    catch(vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (m_frameBufferSizeChanged || acquireImageResult == vk::Result::eErrorOutOfDateKHR)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
        return;
    }

    if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("acquire next image failed");
    }

    m_pCamera->UpdateUniformBuffer();
    m_pLight->UpdateLightUBO();
    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    {
        RHI::VulkanCmdBeginEndRAII cmdBeginEndGuard(m_vkCmds[m_frameIdxInFlight]);

        std::vector<vk::DescriptorSet> tobinding;

        std::vector<vk::ClearValue> clears(2);
        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        if (m_pPhysicalDevice->IsUsingMSAA())
        {
            clears.push_back(clears[0]);
        }
        auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
        auto vkFramebuffer = m_pDevice->GetVulkanPresentFramebuffer(m_imageIdx)->GetVkFramebuffer();

        m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{{0,0},extent}, vkFramebuffer);
        {
            vk::Rect2D rect{{0,0},extent};
            {
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "fill");
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width / 2, (float)extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);
                m_pModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);
            }

            {
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "line");
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{(float)extent.width / 2,0,(float)extent.width / 2, (float)extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);
                m_pModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);
            }
        }
        m_pRenderPass->End(m_vkCmds[m_frameIdxInFlight]);
    }

    // submit
    std::array<vk::PipelineStageFlags, 1> waitStage { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    auto submitInfo = vk::SubmitInfo()
                    .setWaitSemaphores(m_vkSemaphoreImageAvaliables[m_frameIdxInFlight])
                    .setWaitDstStageMask(waitStage)
                    .setCommandBuffers(m_vkCmds[m_frameIdxInFlight])
                    .setSignalSemaphores(m_vkSemaphoreRenderFinisheds[m_frameIdxInFlight]);
    m_pDevice->GetVkGraphicQueue().submit(submitInfo, m_vkFences[m_frameIdxInFlight]);

    // present
    auto presentInfo = vk::PresentInfoKHR()
                    .setImageIndices(m_imageIdx)
                    .setSwapchains(m_pDevice->GetPVulkanSwapchain()->GetSwapchain())
                    .setWaitSemaphores(m_vkSemaphoreRenderFinisheds[m_frameIdxInFlight]);
    vk::Result presentRes;
    try
    {
        presentRes = m_pDevice->GetVkPresentQueue().presentKHR(presentInfo);
    }
    catch(vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR || m_frameBufferSizeChanged)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
    }
    else if ( presentRes!= vk::Result::eSuccess )
    {
        throw std::runtime_error("present image failed");
    }
}


void MultiPipelineRenderer::prepareMultiPipelines()
{
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.frag.spv", vk::ShaderStageFlagBits::eFragment);

    m_Pipelines.resize(2);
    std::shared_ptr<RHI::VulkanViewportState> viewPort;
    auto normalPipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                        .SetVulkanPipelineLayout(m_pPipelineLayout)
                        .SetshaderSet(shaderSet)
                        .buildUnique();

    std::shared_ptr<RHI::VulkanRasterizationState> rasterization = RHI::VulkanRasterizationStateBuilder()
                                                                    .SetPolygonMode(vk::PolygonMode::eLine)
                                                                    .build();
    auto rawLinePipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get(), normalPipeline.get())
                        .SetVulkanRasterizationState(rasterization)
                        .SetVulkanPipelineLayout(m_pPipelineLayout)
                        .SetshaderSet(shaderSet)
                        .buildUnique();


    m_pRenderPass->AddGraphicRenderPipeline("fill", std::move(normalPipeline));
    m_pRenderPass->AddGraphicRenderPipeline("line", std::move(rawLinePipeline));
}