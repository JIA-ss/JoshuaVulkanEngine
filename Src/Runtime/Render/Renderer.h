#pragma once

#include <chrono>
#include <array>
#include <vulkan/vulkan.hpp>
namespace RHI { 
class VulkanDevice;
class VulkanRenderPipeline;
}

namespace Render
{

class Renderer
{
public:
    static const int MAX_FRAMES_IN_FLIGHT = 2;
private:
    RHI::VulkanDevice* m_pRHIDevice;
    vk::Device* m_pVkDevice;
    RHI::VulkanRenderPipeline* m_pRHIRenderPipeline;

    uint32_t m_imageIdx = 0;
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> m_vkCmds;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_vkFenceInFlights;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreImageAvaliables;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreRenderFinisheds;

    std::chrono::steady_clock::time_point m_lastframeTimePoint;
    std::size_t m_frameNum = 0;
    bool m_frameBufferSizeChanged = false;
public:
    Renderer();
    ~Renderer();

    void Render();

private:
    void createCmdBufs();
    void createSyncObjects();
    void destroySyncObjects();
    void frameRateUpdate();
    void recreateSwapchain();
};


}