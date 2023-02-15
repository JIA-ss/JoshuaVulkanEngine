#include "MultiPipelineRenderer.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanViewportState.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"

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
    m_pVulkanPipelineLayout.reset();
}

void MultiPipelineRenderer::prepare()
{
    prepareVertexData();
    prepareShader();
    prepareRenderpass();
    preparePipelineLayout();
    prepareMultiPipelines();
    prepareFrameBuffer();
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
    auto res = m_pDevice->GetVkDevice().acquireNextImageKHR(m_pDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliables[m_frameIdxInFlight], nullptr);
    if (res.result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain();
        return;
    }
    if (res.result != vk::Result::eSuccess && res.result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("acquire next image failed");
    }
    m_imageIdx = res.value;

    updateUniformBuf(m_imageIdx);
    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    {
        RHI::VulkanCmdBeginEndRAII cmdBeginEndGuard(m_vkCmds[m_frameIdxInFlight]);

        m_vkCmds[m_frameIdxInFlight].bindVertexBuffers(0, *m_pVulkanVertexBuffer->GetPVkBuf(), {0});
        m_vkCmds[m_frameIdxInFlight].bindIndexBuffer(*m_pVulkanVertexIndexBuffer->GetPVkBuf(), 0, vk::IndexType::eUint32);
        m_vkCmds[m_frameIdxInFlight].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pVulkanPipelineLayout->GetVkPieplineLayout(), 0, m_pVulkanDescriptorSets->GetVkDescriptorSet(m_frameIdxInFlight), {});


        std::vector<vk::ClearValue> clears(2);

        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        auto renderpassBeginInfo = vk::RenderPassBeginInfo()
                                .setRenderPass(m_pRenderPass->GetVkRenderPass())
                                .setClearValues(clears)
                                .setRenderArea(vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent})
                                .setFramebuffer(m_pDevice->GetSwapchainFramebuffer(m_imageIdx)); 
        {
            RHI::VulkanCmdBeginEndRenderPassRAII cmdBeginEndRenderPassGuard(m_vkCmds[m_frameIdxInFlight], renderpassBeginInfo);
            for (int i = 0; i < m_Pipelines.size(); i++)
            {
                auto& pipeline = m_Pipelines[i];
                // first viewport pipeline
                m_vkCmds[m_frameIdxInFlight].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetVkPipeline());
                auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{i * (float)extent.width / m_Pipelines.size(),0,(float)extent.width / m_Pipelines.size(), (float)extent.height,0,1});
                vk::Rect2D rect{{0,0},extent};
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);
                // m_vkCmds[m_frameIdxInFlight].draw(m_vertices.size(), 1, 0, 0);
                m_vkCmds[m_frameIdxInFlight].drawIndexed(m_indices.size(), 1, 0,0,0);
            }
        }
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

void MultiPipelineRenderer::preparePipelineLayout()
{
    m_pVulkanPipelineLayout = std::make_shared<RHI::VulkanPipelineLayout>(m_pDevice.get(), m_pVulkanDescriptorSetLayout);
}

void MultiPipelineRenderer::prepareMultiPipelines()
{
    m_Pipelines.resize(2);
    std::shared_ptr<RHI::VulkanViewportState> viewPort;
    m_Pipelines[0] = RHI::VulkanRenderPipelineBuilder(m_pDevice.get())
                        .SetdescriptorSetLayout(m_pVulkanDescriptorSetLayout)
                        .SetshaderSet(m_pVulkanShaderSet)
                        .SetVulkanRenderPass(m_pRenderPass)
                        .SetVulkanPipelineLayout(m_pVulkanPipelineLayout)
                        .build();
    std::shared_ptr<RHI::VulkanRasterizationState> rasterization = RHI::VulkanRasterizationStateBuilder()
                                                                    .SetPolygonMode(vk::PolygonMode::eLine)
                                                                    .build();
    m_Pipelines[1] = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_Pipelines[0])
                        .SetVulkanRasterizationState(rasterization)
                        .SetdescriptorSetLayout(m_pVulkanDescriptorSetLayout)
                        .SetshaderSet(m_pVulkanShaderSet)
                        .SetVulkanRenderPass(m_pRenderPass)
                        .SetVulkanPipelineLayout(m_pVulkanPipelineLayout)
                        .build();
}