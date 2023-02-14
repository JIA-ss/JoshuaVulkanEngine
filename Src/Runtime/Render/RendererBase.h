#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include <memory>
#include <Runtime/VulkanRHI/VulkanInstance.h>
#include <Runtime/VulkanRHI/VulkanPhysicalDevice.h>
#include <Runtime/VulkanRHI/VulkanDevice.h>


namespace Render {

class RendererBase
{
protected:
    std::unique_ptr<RHI::VulkanInstance> m_pInstance;
    std::unique_ptr<RHI::VulkanPhysicalDevice> m_pPhysicalDevice;
    std::unique_ptr<RHI::VulkanDevice> m_pDevice;

    std::unique_ptr<RHI::VulkanRenderPass> m_pRenderPass;
protected:
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> m_vkCmds;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_vkFences;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreImageAvaliables;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreRenderFinisheds;

    uint32_t m_frameIdxInFlight = 0;

    std::chrono::steady_clock::time_point m_lastframeTimePoint;
    std::size_t m_frameNum = 0;
    bool m_frameBufferSizeChanged = false;

public:
    explicit RendererBase(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    virtual ~RendererBase();

    void RenderLoop();


protected:
    virtual void prepare() = 0;
    virtual void render() = 0;

    void recreateSwapchain();
private:

    void initCmd();
    void initSyncObj();
    void initFrameBufferResizeCallback();

    void unInitCmd();
    void unInitSyncObj();

    void outputFrameRate();
};


}