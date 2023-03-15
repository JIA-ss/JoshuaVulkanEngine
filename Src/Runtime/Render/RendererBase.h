#pragma once
#include "Runtime/Platform/PlatformWindow.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <functional>
#include <list>
#include <memory>
#include <Runtime/VulkanRHI/VulkanInstance.h>
#include <Runtime/VulkanRHI/VulkanPhysicalDevice.h>
#include <Runtime/VulkanRHI/VulkanDevice.h>


namespace Render {

class RendererBase
{
protected:
    struct PresentFramebufferAttachmentResource
    {
        // 0. present color
        // 1. depth
        std::unique_ptr<RHI::VulkanImageResource> depthVulkanImageResource;
        // 2. supersample
        std::unique_ptr<RHI::VulkanImageResource> superSampleVulkanImageResource;
    };
    PresentFramebufferAttachmentResource m_presentFramebufferAttachResource;
protected:
    std::unique_ptr<RHI::VulkanInstance> m_pInstance;
    std::unique_ptr<RHI::VulkanPhysicalDevice> m_pPhysicalDevice;
    std::unique_ptr<RHI::VulkanDevice> m_pDevice;

    std::weak_ptr<RHI::VulkanDescriptorSetLayout> m_pSet0UniformSetLayout;
    std::weak_ptr<RHI::VulkanDescriptorSetLayout> m_pSet1SamplerSetLayout;
    std::weak_ptr<RHI::VulkanDescriptorSetLayout> m_pSet2ShadowmapSamplerLayout;
    std::shared_ptr<RHI::VulkanPipelineLayout> m_pPipelineLayout;

    std::shared_ptr<RHI::VulkanRenderPass> m_pRenderPass;

protected:
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> m_vkCmds;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_vkFences;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreImageAvaliables;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreRenderFinisheds;

    std::vector<RHI::VulkanFramebuffer::Attachment> m_VulkanPresentFramebufferAttachments;

    uint32_t m_frameIdxInFlight = 0;

    std::chrono::steady_clock::time_point m_lastframeTimePoint;
    std::size_t m_frameNum = 0;
    bool m_frameBufferSizeChanged = false;

    bool m_skipRender = false;
private:
    std::vector<std::function<void()>> m_enterRenderLoopCallbacks;
    std::vector<std::function<void()>> m_quiteRenderLoopCallbacks;

    std::vector<std::function<void()>> m_preRenderFrameCallbacks;
    std::vector<std::function<void()>> m_postRenderFrameCallbacks;


public:
    explicit RendererBase(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    virtual ~RendererBase();

    void PreRenderLoop();
    void PostRenderLoop();
    void RenderFrame();
    void RenderLoop();

    void AddEnterRenderLoopCallback(std::function<void()> cb) { m_enterRenderLoopCallbacks.push_back(cb); }
    void AddQuiteRenderLoopCallback(std::function<void()> cb) { m_quiteRenderLoopCallbacks.push_back(cb); }
    void AddPreRenderFrameCallback(std::function<void()> cb) { m_preRenderFrameCallbacks.push_back(cb); }
    void AddPostRenderFrameCallback(std::function<void()> cb) { m_postRenderFrameCallbacks.push_back(cb); }

    void StartRender() { m_skipRender = false; }
    void StopRender() { m_skipRender = true; }
    bool IsRendering() { return !m_skipRender; }
    platform::PlatformWindow* GetPWindow() { return m_pPhysicalDevice->GetConfig().window; }

    virtual std::vector<RHI::Model*> GetModels() = 0;
    virtual Camera* GetCamera() = 0;
    virtual Lights* GetLights() { return nullptr; }

    static std::shared_ptr<RendererBase> StartUpRenderer(const std::string& demoName, const RHI::VulkanInstance::Config& instanceConfig, const RHI::VulkanPhysicalDevice::Config& physicalConfig);
protected:
    virtual void prepare() = 0;

    // 1. layout
    virtual void prepareLayout() = 0;
    // 2. attachments
    virtual void preparePresentFramebufferAttachments();
    // 3. renderpass
    virtual void prepareRenderpass();
    // 4. framebuffer
    virtual void preparePresentFramebuffer();
    // 5. pipeline
    virtual void preparePipeline() = 0;

    // for recreate swapchain, need fill present vkImageView into the attachments
    virtual int getPresentImageAttachmentId() { return 0; };

    virtual void render() = 0;
    vk::CommandBuffer& beginCommand();
    void endCommand(vk::CommandBuffer& cmd);

    void recreateSwapchain();
    void prepareDescriptorLayout();
private:

    void initCmd();
    void initSyncObj();
    void initFrameBufferResizeCallback();

    void unInitCmd();
    void unInitSyncObj();

    void outputFrameRate();
};

class RendererList
{
private:
    std::list<RendererBase*> m_renderers;
public:
    explicit RendererList(const std::vector<RendererBase*>& renderers);
    void RenderLoop();
private:
    void renderFrame();
    void prepareRenderLoop();

};

}