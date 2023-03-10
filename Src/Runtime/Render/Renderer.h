#pragma once

#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
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
private:
    RHI::VulkanDevice* m_pRHIDevice;
    vk::Device* m_pVkDevice;
    RHI::VulkanRenderPipeline* m_pRHIRenderPipeline;
    RHI::VulkanDescriptorSets* m_pRHIUniformDescSets;
    RHI::VulkanDescriptorSets* m_pRHISamplerDescSets;

    std::vector<RHI::Vertex> m_vertices;
    std::unique_ptr<RHI::VulkanVertexBuffer> m_pVulkanVertexBuffer;

    std::vector<uint32_t> m_indices;
    std::unique_ptr<RHI::VulkanVertexIndexBuffer> m_pVulkanVertexIndexBuffer;

    uint32_t m_imageIdx = 0;
    std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT> m_vkCmds;
    std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_vkFenceInFlights;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreImageAvaliables;
    std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_vkSemaphoreRenderFinisheds;

    std::chrono::steady_clock::time_point m_lastframeTimePoint;
    std::size_t m_frameNum = 0;
    bool m_frameBufferSizeChanged = false;

    static uint32_t s_frameIdxInFlight;
public:
    static uint32_t GetCurrentFrameIndex() { return s_frameIdxInFlight; };

    Renderer();
    ~Renderer();

    void Render();

private:
    void createVertices();
    void createVertexBuf();
    void createIndiciesBuf();


    void createCmdBufs();
    void createSyncObjects();
    void destroySyncObjects();
    void frameRateUpdate();
    void updateUniformBuf(uint32_t currentFrameIdx);
    void recreateSwapchain();
};


}