#pragma once

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
private:
    RHI::VulkanDevice* m_pRHIDevice;
    vk::Device* m_pVkDevice;
    RHI::VulkanRenderPipeline* m_pRHIRenderPipeline;

    vk::CommandBuffer m_vkCmd;
    uint32_t m_imageIdx = 0;

    vk::RenderPassBeginInfo m_vkRenderpassBeginInfo;

    vk::Fence m_vkFenceInFlight;
    vk::Semaphore m_vkSemaphoreImageAvaliable;
    vk::Semaphore m_vkSemaphoreRenderFinished;
public:
    Renderer();
    ~Renderer();

    void Render();
};


}