#include "RendererBase.h"
#include "Runtime/Render/SimpleModel/SimpleModelRenderer.h"
#include "Runtime/Render/MultiPipelines/MultiPipelineRenderer.h"
#include "Runtime/Render/Deferred/DeferredRenderer.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/Render/ShadowMap/ShadowMapRenderer.h"
#include "Runtime/Render/PBR/PBRRenderer.h"
#include "vulkan/vulkan_enums.hpp"
#include <Runtime/VulkanRHI/VulkanShaderSet.h>
#include <iostream>
#include <memory>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
using namespace Render;

std::shared_ptr<RendererBase> RendererBase::StartUpRenderer(const std::string& demoName, const RHI::VulkanInstance::Config& instanceConfig, const RHI::VulkanPhysicalDevice::Config& physicalConfig)
{
    static std::vector<std::string> targetDemoName;
#define MAKE_SHARED_WITH_NAME(_NAME_)   \
    targetDemoName.push_back(#_NAME_);  \
    if (demoName == #_NAME_) {  \
        return std::make_shared<_NAME_##Renderer>(instanceConfig, physicalConfig);  \
    }

    MAKE_SHARED_WITH_NAME(SimpleModel)
    MAKE_SHARED_WITH_NAME(MultiPipeline)
    MAKE_SHARED_WITH_NAME(ShadowMap)
    MAKE_SHARED_WITH_NAME(PBR)
    MAKE_SHARED_WITH_NAME(Deferred)

    std::cout << "!!!!arg error!!!!" << std::endl
                << "please input arg as the following demo name: " << std::endl;
    for (auto& name : targetDemoName)
    {
        std::cout << name << std::endl;
    }
    assert(false);
    return nullptr;

#undef MAKE_SHARED_WITH_NAME
}

RendererBase::RendererBase(const RHI::VulkanInstance::Config& instanceConfig,
    const RHI::VulkanPhysicalDevice::Config& physicalConfig)
{
    ZoneScopedN("RendererBase::RendererBase");
    m_pInstance.reset(new RHI::VulkanInstance(instanceConfig));
    m_pPhysicalDevice.reset(new RHI::VulkanPhysicalDevice(physicalConfig, m_pInstance.get()));
    m_pDevice.reset(new RHI::VulkanDevice(m_pPhysicalDevice.get()));

    initCmd();
    initSyncObj();
    initFrameBufferResizeCallback();
    prepareDescriptorLayout();

    m_lastframeTimePoint = std::chrono::high_resolution_clock::now();
}

void RendererBase::PreRenderLoop()
{
    prepare();
    assert(m_pRenderPass);

    for(auto& cb : m_enterRenderLoopCallbacks)
    {
        cb();
    }
}

void RendererBase::PostRenderLoop()
{
    for(auto& cb : m_quiteRenderLoopCallbacks)
    {
        cb();
    }
}

void RendererBase::RenderFrame()
{
    GetPWindow()->PollWindowEvent();

    if (!IsRendering())
    {
        return;
    }

    for(auto& cb : m_preRenderFrameCallbacks)
    {
        cb();
    }

    render();
    outputFrameRate();
    m_frameIdxInFlight = (m_frameIdxInFlight + 1) % MAX_FRAMES_IN_FLIGHT;

    for(auto& cb : m_postRenderFrameCallbacks)
    {
        cb();
    }
}

void RendererBase::RenderLoop()
{
    PreRenderLoop();


    while (!GetPWindow()->ShouldClose())
    {
        RenderFrame();
    }

    PostRenderLoop();
}


RendererBase::~RendererBase()
{
    unInitCmd();
    unInitSyncObj();
    m_pRenderPass = nullptr;
    m_pPipelineLayout.reset();
    m_presentFramebufferAttachResource.depthVulkanImageResource.reset();
    m_presentFramebufferAttachResource.superSampleVulkanImageResource.reset();
    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}


void RendererBase::preparePresentFramebufferAttachments()
{
    ZoneScopedN("RendererBase::preparePresentFramebufferAttachments");
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();

    auto sampleCount = m_pPhysicalDevice->GetSampleCount();
    RHI::VulkanImageResource::Config imageConfig;
    imageConfig.extent = vk::Extent3D{ m_pDevice->GetSwapchainExtent(), 1};
    imageConfig.sampleCount = sampleCount;

    // present
    {
    }

    // depth
    {
        imageConfig.format = m_pPhysicalDevice->QuerySupportedDepthFormat();
        imageConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        if (m_pPhysicalDevice->HasStencilComponent(imageConfig.format))
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        }
        else
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        }
        m_presentFramebufferAttachResource.depthVulkanImageResource.reset(new RHI::VulkanImageResource(m_pDevice.get(), vk::MemoryPropertyFlagBits::eDeviceLocal, imageConfig));
    }

    // supersample color
    if (m_pPhysicalDevice->IsUsingMSAA())
    {
        imageConfig.format = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
        imageConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        imageConfig.subresourceRange
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        ;
        m_presentFramebufferAttachResource.superSampleVulkanImageResource.reset(new RHI::VulkanImageResource(m_pDevice.get(), vk::MemoryPropertyFlagBits::eDeviceLocal, imageConfig));
    }

    m_VulkanPresentFramebufferAttachments.resize(m_pPhysicalDevice->IsUsingMSAA() ? 3 : 2);
    // present
    m_VulkanPresentFramebufferAttachments[0].type = m_pPhysicalDevice->IsUsingMSAA() ? RHI::VulkanFramebuffer::kResolve : RHI::VulkanFramebuffer::kColor;
    m_VulkanPresentFramebufferAttachments[0].samples = vk::SampleCountFlagBits::e1;
    m_VulkanPresentFramebufferAttachments[0].resourceFinalLayout = vk::ImageLayout::ePresentSrcKHR;
    m_VulkanPresentFramebufferAttachments[0].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[0].resourceFormat = colorFormat;

    // depth
    m_VulkanPresentFramebufferAttachments[1].type = RHI::VulkanFramebuffer::kDepthStencil;
    m_VulkanPresentFramebufferAttachments[1].samples = sampleCount;
    m_VulkanPresentFramebufferAttachments[1].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[1].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[1].resource = m_presentFramebufferAttachResource.depthVulkanImageResource->GetNative();
    m_VulkanPresentFramebufferAttachments[1].resourceFormat = depthForamt;

    // superSample
    if (m_pPhysicalDevice->IsUsingMSAA())
    {
        m_VulkanPresentFramebufferAttachments[2].type = RHI::VulkanFramebuffer::kColor;
        m_VulkanPresentFramebufferAttachments[2].samples = sampleCount;
        m_VulkanPresentFramebufferAttachments[2].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
        m_VulkanPresentFramebufferAttachments[2].resourceFinalLayout = vk::ImageLayout::eColorAttachmentOptimal;
        m_VulkanPresentFramebufferAttachments[2].resource = m_presentFramebufferAttachResource.superSampleVulkanImageResource->GetNative();
        m_VulkanPresentFramebufferAttachments[2].resourceFormat = colorFormat;
    }

}

void RendererBase::prepareRenderpass()
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice.get())
                            .SetAttachments(m_VulkanPresentFramebufferAttachments)
                            .buildUnique();
}

void RendererBase::preparePresentFramebuffer()
{
    ZoneScopedN("RendererBase::preparePresentFramebuffer");
    m_pDevice->CreateVulkanPresentFramebuffer(
        m_pRenderPass.get(),
        m_pDevice->GetSwapchainExtent().width,
        m_pDevice->GetSwapchainExtent().height,
        1,
        m_VulkanPresentFramebufferAttachments,
        getPresentImageAttachmentId()
    );
}

void RendererBase::initCmd()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_vkCmds[i] = m_pDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
        m_tracyVkCtx[i] = TracyVkContext(m_pPhysicalDevice->GetVkPhysicalDevice(), m_pDevice->GetVkDevice(), m_pDevice->GetVkGraphicQueue(), m_vkCmds[i]);
    }
}

void RendererBase::initSyncObj()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::FenceCreateInfo fenceInfo;
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        m_vkFences[i] = m_pDevice->GetVkDevice().createFence(fenceInfo);

        vk::SemaphoreCreateInfo semaphoreInfo;
        m_vkSemaphoreImageAvaliables[i] = m_pDevice->GetVkDevice().createSemaphore(semaphoreInfo);
        m_vkSemaphoreRenderFinisheds[i] = m_pDevice->GetVkDevice().createSemaphore(semaphoreInfo);
    }
}

void RendererBase::initFrameBufferResizeCallback()
{
    m_pDevice->GetVulkanPhysicalDevice()
                ->GetPWindow()
                    ->AddFrameBufferSizeChangedCallback([&](int width, int height)
                        {
                            m_frameBufferSizeChanged = true;
                        });
}


void RendererBase::unInitCmd()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_pDevice->GetPVulkanCmdPool()->FreeReUsableCmd(m_vkCmds[i]);
        m_vkCmds[i] = nullptr;

        TracyVkDestroy(m_tracyVkCtx[i]);
        m_tracyVkCtx[i] = nullptr;
    }
}

void RendererBase::unInitSyncObj()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto res = m_pDevice->GetVkDevice().waitForFences(m_vkFences[i], true, std::numeric_limits<uint64_t>::max());
        assert(res == vk::Result::eSuccess);
        m_pDevice->GetVkDevice().destroyFence(m_vkFences[i]);
        m_pDevice->GetVkDevice().destroySemaphore(m_vkSemaphoreImageAvaliables[i]);
        m_pDevice->GetVkDevice().destroySemaphore(m_vkSemaphoreRenderFinisheds[i]);
        m_vkSemaphoreImageAvaliables[i] = nullptr;
        m_vkSemaphoreRenderFinisheds[i] = nullptr;
        m_vkFences[i] = nullptr;
    }
}

void RendererBase::outputFrameRate()
{
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

void RendererBase::recreateSwapchain()
{
    auto swapchainExtent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
    auto windowSetting = m_pDevice->GetVulkanPhysicalDevice()->GetPWindow()->GetWindowSetting();
    m_pDevice->GetVulkanPhysicalDevice()->GetPWindow()->WaitIfMinimization();
    m_pDevice->GetVkDevice().waitIdle();
    if (swapchainExtent.width == windowSetting.width && swapchainExtent.height == windowSetting.height)
    {
        return;
    }

    m_pDevice->ReCreateSwapchain(m_pRenderPass.get(), windowSetting.width, windowSetting.height, 1, m_VulkanPresentFramebufferAttachments, getPresentImageAttachmentId());
}

void RendererBase::prepareDescriptorLayout()
{
    m_pSet0UniformSetLayout = m_pDevice->GetDescLayoutPresets().UBO;
    m_pSet1SamplerSetLayout = m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER;
    m_pSet2ShadowmapSamplerLayout = m_pDevice->GetDescLayoutPresets().SHADOWMAP;
}








RendererList::RendererList(const std::vector<RendererBase*>& renderers)
{
    m_renderers.insert(m_renderers.end(), renderers.begin(), renderers.end());
}

void RendererList::prepareRenderLoop()
{
    for (auto& render : m_renderers)
    {
        render->PreRenderLoop();
    }
}

void RendererList::renderFrame()
{
    auto it = m_renderers.begin();
    while (it != m_renderers.end())
    {
        RendererBase* renderer = *it;
        if (renderer == nullptr)
        {
            it = m_renderers.erase(it);
            continue;
        }

        if (!renderer->GetPWindow()->ShouldClose())
        {
            renderer->RenderFrame();
        }
        else
        {
            it = m_renderers.erase(it);
            renderer->PostRenderLoop();
            continue;
        }

        it++;
    }
}

void RendererList::RenderLoop()
{
    prepareRenderLoop();
    while (!m_renderers.empty())
    {
        renderFrame();
        FrameMark;
    }
}