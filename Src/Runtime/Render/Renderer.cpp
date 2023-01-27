#include "Renderer.h"
#include "Runtime/Render/Renderer.h"
#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <limits>
#include <stdexcept>

namespace Render
{

Renderer::Renderer()
{
    m_pRHIDevice = &RHI::VulkanContext::GetInstance().GetVulkanDevice();
    m_pVkDevice = &m_pRHIDevice->GetVkDevice();
    m_pRHIRenderPipeline = &RHI::VulkanContext::GetInstance().GetVulkanRenderPipeline();
    m_vkCmd = m_pRHIDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
    vk::ClearValue clear{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
    m_vkRenderpassBeginInfo.setRenderPass(m_pRHIRenderPipeline->GetVkRenderPass())
                            .setFramebuffer(m_pRHIDevice->GetPVulkanSwapchain()->GetFramebuffer(m_imageIdx))
                            .setRenderArea(vk::Rect2D{vk::Offset2D{0,0}, m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent})
                            .setClearValues(clear);

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    m_vkFenceInFlight = m_pVkDevice->createFence(fenceInfo);

    vk::SemaphoreCreateInfo semaphoreInfo;
    m_vkSemaphoreImageAvaliable = m_pVkDevice->createSemaphore(semaphoreInfo);
    m_vkSemaphoreRenderFinished = m_pVkDevice->createSemaphore(semaphoreInfo);
}

Renderer::~Renderer()
{
    m_pVkDevice->destroyFence(m_vkFenceInFlight);
    m_pVkDevice->destroySemaphore(m_vkSemaphoreImageAvaliable);
    m_pVkDevice->destroySemaphore(m_vkSemaphoreRenderFinished);
}

void Renderer::Render()
{
    /*
    
    # Rendering a frame in Vulkan consists of a common set of steps:

    - Wait for the previous frame to finish
    - Acquire an image from the swap chain
    - Record a command buffer which draws the scene onto that image
    - Submit the recorded command buffer
    - Present the swap chain image

    */

    // wait for fence
    if (
        m_pVkDevice->waitForFences(m_vkFenceInFlight, true, std::numeric_limits<uint64_t>::max())
        !=
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("wait for inflight fence failed");
    }
    m_pVkDevice->resetFences(m_vkFenceInFlight);

    // acquire image
    auto res = m_pVkDevice->acquireNextImageKHR(m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliable, nullptr);
    if (res.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("acquire next image failed");
    }
    m_imageIdx = res.value;


    // record command buffer
    m_vkCmd.reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmd.begin(beginInfo);
    {
        m_vkCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pRHIRenderPipeline->GetVkPipeline());
        m_pRHIRenderPipeline->GetPVulkanDynamicState()->SetUpCmdBuf(m_vkCmd);
        m_vkRenderpassBeginInfo.setRenderArea(vk::Rect2D{vk::Offset2D{0,0}, m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent})
                                .setFramebuffer(m_pRHIDevice->GetPVulkanSwapchain()->GetFramebuffer(m_imageIdx)); 
        m_vkCmd.beginRenderPass(m_vkRenderpassBeginInfo, {});
        {
            m_vkCmd.draw(3, 1, 0, 0);
        }
        m_vkCmd.endRenderPass();
    }
    m_vkCmd.end();

    // submit
    std::array<vk::PipelineStageFlags, 1> waitStage { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    auto submitInfo = vk::SubmitInfo()
                    .setWaitSemaphores(m_vkSemaphoreImageAvaliable)
                    .setWaitDstStageMask(waitStage)
                    .setCommandBuffers(m_vkCmd)
                    .setSignalSemaphores(m_vkSemaphoreRenderFinished);
    m_pRHIDevice->GetVkGraphicQueue().submit(submitInfo, m_vkFenceInFlight);

    // present
    auto presentInfo = vk::PresentInfoKHR()
                    .setImageIndices(m_imageIdx)
                    .setSwapchains(m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchain())
                    .setWaitSemaphores(m_vkSemaphoreRenderFinished);
    if (
        m_pRHIDevice->GetVkPresentQueue().presentKHR(presentInfo)
        != 
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("present image failed");
    }
}

}