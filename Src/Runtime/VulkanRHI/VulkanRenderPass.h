#pragma once
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"
#include <vector>
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

class VulkanRenderPassBuilder
{
public:
    explicit VulkanRenderPassBuilder(VulkanDevice* device) : m_pDevice(device) { }

    inline VulkanRenderPassBuilder& SetAttachments(const std::vector<VulkanFramebuffer::Attachment>& attachments) { m_attachments = attachments; return *this; }
    VulkanRenderPassBuilder& SetDefaultSubpass();
    VulkanRenderPassBuilder& SetDefaultSubpassDependencies();
    inline VulkanRenderPassBuilder& AddSubpass(vk::SubpassDescription subpass) { m_subpasses.push_back(subpass); return *this; }
    inline VulkanRenderPassBuilder& AddSubpassDependency(vk::SubpassDependency dep) { m_dependencies.push_back(dep); return *this; }

    std::unique_ptr<VulkanRenderPass> buildUnique();
private:
    VulkanDevice* m_pDevice;
    std::vector<VulkanFramebuffer::Attachment> m_attachments;
    std::vector<vk::SubpassDescription> m_subpasses;
    std::vector<vk::SubpassDependency> m_dependencies;

    std::vector<vk::AttachmentReference> m_vkColorAttachments;
    std::vector<vk::AttachmentReference> m_vkDepthStencilAttachments;
    std::vector<vk::AttachmentReference> m_vkResolveAttachments;
};
RHI_NAMESPACE_END