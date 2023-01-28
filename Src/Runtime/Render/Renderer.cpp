#include "Renderer.h"
#include "Runtime/Render/Renderer.h"
#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <chrono>
#include <limits>
#include <iostream>
#include <ratio>
#include <stdexcept>

namespace Render
{

Renderer::Renderer()
{
    m_pRHIDevice = &RHI::VulkanContext::GetInstance().GetVulkanDevice();
    m_pVkDevice = &m_pRHIDevice->GetVkDevice();
    m_pRHIRenderPipeline = &RHI::VulkanContext::GetInstance().GetVulkanRenderPipeline();
    createCmdBufs();
    createSyncObjects();

    // recreate swapchain
    auto platformWindow = m_pRHIDevice->GetVulkanPhysicalDevice()->GetPWindow();
    platformWindow->AddFrameBufferSizeChangedCallback([&](int width, int height)
    {
        m_frameBufferSizeChanged = true;
    });
}

Renderer::~Renderer()
{
    destroySyncObjects();
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

    static int s_frameIdxInFlight = 0;

    // wait for fence
    if (
        m_pVkDevice->waitForFences(m_vkFenceInFlights[s_frameIdxInFlight], true, std::numeric_limits<uint64_t>::max())
        !=
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("wait for inflight fence failed");
    }

    // acquire image
    auto res = m_pVkDevice->acquireNextImageKHR(m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliables[s_frameIdxInFlight], nullptr);
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

    // reset fence after acquiring the image
    m_pVkDevice->resetFences(m_vkFenceInFlights[s_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[s_frameIdxInFlight].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[s_frameIdxInFlight].begin(beginInfo);
    {
        m_vkCmds[s_frameIdxInFlight].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pRHIRenderPipeline->GetVkPipeline());
        m_pRHIRenderPipeline->GetPVulkanDynamicState()->SetUpCmdBuf(m_vkCmds[s_frameIdxInFlight]);
        vk::ClearValue clear{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        auto renderpassBeginInfo = vk::RenderPassBeginInfo()
                                .setRenderPass(m_pRHIRenderPipeline->GetVkRenderPass())
                                .setClearValues(clear)
                                .setRenderArea(vk::Rect2D{vk::Offset2D{0,0}, m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent})
                                .setFramebuffer(m_pRHIDevice->GetSwapchainFramebuffer(m_imageIdx)); 
        m_vkCmds[s_frameIdxInFlight].beginRenderPass(renderpassBeginInfo , {});
        {
            m_vkCmds[s_frameIdxInFlight].draw(3, 1, 0, 0);
        }
        m_vkCmds[s_frameIdxInFlight].endRenderPass();
    }
    m_vkCmds[s_frameIdxInFlight].end();

    // submit
    std::array<vk::PipelineStageFlags, 1> waitStage { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    auto submitInfo = vk::SubmitInfo()
                    .setWaitSemaphores(m_vkSemaphoreImageAvaliables[s_frameIdxInFlight])
                    .setWaitDstStageMask(waitStage)
                    .setCommandBuffers(m_vkCmds[s_frameIdxInFlight])
                    .setSignalSemaphores(m_vkSemaphoreRenderFinisheds[s_frameIdxInFlight]);
    m_pRHIDevice->GetVkGraphicQueue().submit(submitInfo, m_vkFenceInFlights[s_frameIdxInFlight]);

    // present
    auto presentInfo = vk::PresentInfoKHR()
                    .setImageIndices(m_imageIdx)
                    .setSwapchains(m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchain())
                    .setWaitSemaphores(m_vkSemaphoreRenderFinisheds[s_frameIdxInFlight]);
    auto presentRes = m_pRHIDevice->GetVkPresentQueue().presentKHR(presentInfo);
    if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR || m_frameBufferSizeChanged)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
    }
    else if ( presentRes!= vk::Result::eSuccess )
    {
        throw std::runtime_error("present image failed");
    }



    frameRateUpdate();
    s_frameIdxInFlight = (s_frameIdxInFlight + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::frameRateUpdate()
{
    // frame rate
    m_frameNum++;
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_ms = now - m_lastframeTimePoint;
    double time_ms = duration_ms.count();
    if (time_ms >= 500)
    {
        double time_s = time_ms / (double)1000;
        double fps = (double)m_frameNum / time_s;
        std::cout << "fps:\t" << fps << "\r";
        m_lastframeTimePoint = now;
        m_frameNum = 0;
    }
}
void Renderer::createCmdBufs()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_vkCmds[i] = m_pRHIDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
    }
}

void Renderer::createSyncObjects()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::FenceCreateInfo fenceInfo;
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        m_vkFenceInFlights[i] = m_pVkDevice->createFence(fenceInfo);

        vk::SemaphoreCreateInfo semaphoreInfo;
        m_vkSemaphoreImageAvaliables[i] = m_pVkDevice->createSemaphore(semaphoreInfo);
        m_vkSemaphoreRenderFinisheds[i] = m_pVkDevice->createSemaphore(semaphoreInfo);
    }
    m_lastframeTimePoint = std::chrono::high_resolution_clock::now();
}

void Renderer::destroySyncObjects()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto res = m_pVkDevice->waitForFences(m_vkFenceInFlights[i], true, std::numeric_limits<uint64_t>::max());
        assert(res == vk::Result::eSuccess);
        m_pVkDevice->resetFences(m_vkFenceInFlights[i]);
        m_pVkDevice->destroyFence(m_vkFenceInFlights[i]);
        m_pVkDevice->destroySemaphore(m_vkSemaphoreImageAvaliables[i]);
        m_pVkDevice->destroySemaphore(m_vkSemaphoreRenderFinisheds[i]);
    }
}

void Renderer::recreateSwapchain()
{
    auto swapchainExtent = m_pRHIDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
    auto windowSetting = m_pRHIDevice->GetVulkanPhysicalDevice()->GetPWindow()->GetWindowSetting();
    m_pRHIDevice->GetVulkanPhysicalDevice()->GetPWindow()->WaitIfMinimization();
    m_pVkDevice->waitIdle();
    if (swapchainExtent.width == windowSetting.width && swapchainExtent.height == windowSetting.height)
    {
        return;
    }

    m_pRHIDevice->ReCreateSwapchain(m_pRHIRenderPipeline);
}
}