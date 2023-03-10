#include "RendererBase.h"
#include "Runtime/Render/SimpleModel/SimpleModelRenderer.h"
#include "Runtime/Render/MultiPipelines/MultiPipelineRenderer.h"
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

    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}


void RendererBase::initCmd()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_vkCmds[i] = m_pDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
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

    m_pDevice->ReCreateSwapchain(m_pRenderPass.get());
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
    }
}