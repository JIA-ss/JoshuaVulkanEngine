#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
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


    uint32_t m_frameIdxInFlight = 0;

    std::chrono::steady_clock::time_point m_lastframeTimePoint;
    std::size_t m_frameNum = 0;
    bool m_frameBufferSizeChanged = false;

public:
    explicit RendererBase(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    virtual ~RendererBase();

    void RenderLoop();

    static std::shared_ptr<RendererBase> StartUpRenderer(const std::string& demoName, const RHI::VulkanInstance::Config& instanceConfig, const RHI::VulkanPhysicalDevice::Config& physicalConfig);
protected:
    virtual void prepare() = 0;
    virtual void prepareRenderpass() = 0;
    virtual void render() = 0;

    vk::CommandBuffer& beginCommand();
    void endCommand(vk::CommandBuffer& cmd);

    void recreateSwapchain();

    virtual void prepareLayout();
private:

    void initCmd();
    void initSyncObj();
    void initFrameBufferResizeCallback();

    void unInitCmd();
    void unInitSyncObj();

    void outputFrameRate();
};


}