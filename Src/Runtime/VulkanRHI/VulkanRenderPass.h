#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

#include <map>
RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanRenderPipeline;
class VulkanRenderPass
{
private:
    struct VulkanPipelineWrapper
    {
        std::unique_ptr<VulkanRenderPipeline> pipeline;
        std::string name;
    };
private:
    VulkanDevice* m_vulkanDevice;

    vk::RenderPass m_vkRenderPass;
    vk::Format m_colorFormat;
    vk::Format m_depthFormat;

    std::vector<VulkanPipelineWrapper> m_graphicPipelines;
public:
    VulkanRenderPass(VulkanDevice* device, vk::Format colorFormat, vk::Format depthFormat, vk::SampleCountFlagBits sample);
    VulkanRenderPass(VulkanDevice* device, vk::RenderPass renderpass);
    ~VulkanRenderPass();

    void Begin(vk::CommandBuffer cmd, const std::vector<vk::ClearValue>& clearValues, const vk::Rect2D& renderArea, vk::Framebuffer frameBuffer, vk::SubpassContents contents = {});
    void End(vk::CommandBuffer cmd);

    void BindGraphicPipeline(vk::CommandBuffer cmd, const std::string& name);

    inline vk::RenderPass& GetVkRenderPass() { return m_vkRenderPass; }
    void AddGraphicRenderPipeline(const std::string& name, std::unique_ptr<VulkanRenderPipeline> pipeline);
    VulkanRenderPipeline* GetGraphicRenderPipeline(const std::string& name);
};
RHI_NAMESPACE_END