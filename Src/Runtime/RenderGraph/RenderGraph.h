#pragma once

#include "Runtime/RenderGraph/Pass/RenderGraphPassExecutor.h"
#include "Runtime/RenderGraph/RenderDependencyGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResource.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
#include <memory>
#include <optional>
#include <stdint.h>
#include <vcruntime.h>
#include <vector>
namespace Render
{

class RenderGraphBuilder;
class RenderGraphRegistry;
class RenderGraph
{
    friend class RenderGraphBuilder;
    friend class RenderGraphRegistry;

public:
    explicit RenderGraph(RHI::VulkanDevice* device);
    ~RenderGraph();

public:
    void compile();
    void execute();

private:
    class CompileFramebuffer
    {
    public:
        inline RHI::VulkanFramebuffer* getFramebuffer(RenderGraphPassNode* passNode)
        {
            return passToFrameBuffer[passNode];
        }
        void addPresentFramebuffer(RenderGraphPassNode* passNode, RHI::VulkanFramebuffer* framebuffer)
        {
            passToFrameBuffer[passNode] = framebuffer;
        }
        void addFramebuffer(RenderGraphPassNode* passNode, std::unique_ptr<RHI::VulkanFramebuffer> framebuffer)
        {
            passToFrameBuffer[passNode] = framebuffer.get();
            framebuffers.push_back(std::move(framebuffer));
        }

    private:
        std::vector<std::unique_ptr<RHI::VulkanFramebuffer>> framebuffers;
        std::map<RenderGraphPassNode*, RHI::VulkanFramebuffer*> passToFrameBuffer;
    };

    struct CompiledRenderPass
    {
        size_t headPassIndex;
        size_t tailPassIndex;
        RHI::VulkanPipelineLayout::BindingInfo bindingInfo;

        std::shared_ptr<RHI::VulkanPipelineLayout> pipelineLayout;
        std::unique_ptr<RHI::VulkanRenderPass> renderPass;
        std::unique_ptr<CompileFramebuffer> compiledFramebuffer;

        std::unique_ptr<RHI::VulkanDescriptorPool> descriptorPool;
        std::map<int, std::vector<std::shared_ptr<RHI::VulkanDescriptorSets>>> passIdxToMeshPassIdxToDescriptorSets;
    };

    struct VkSyncObjects
    {
        std::vector<vk::CommandBuffer> cmds;
        std::vector<vk::Fence> fences;
        std::vector<vk::Semaphore> semaphoreImageAvaliables;
        std::vector<vk::Semaphore> semaphoreRenderFinisheds;
    };

private:
    void cullNodes();
    void mergePass();
    void compileDiscriptorsets();
    void createDiscriptorPool(CompiledRenderPass& compiledRenderPass);
    int getValidPassNum(int headIndex, int tailIndex);
    void compileRenderPass(int headIndex, int tailIndex, const RHI::VulkanPipelineLayout::BindingInfo& bindingInfo);
    void updatePassNodeAttachments(
        RenderGraphPassNode* passNode, const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void createLayout(CompiledRenderPass& compiledRenderPass);
    void createRenderpass(
        CompiledRenderPass& compiledRenderPass, const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments);
    void createRenderPipeline(CompiledRenderPass& compiledRenderPass);
    void compileFramebuffer(
        RenderGraphPassNode* passNode, CompiledRenderPass& compiledRenderPass,
        const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments, uint32_t attachmentWidth,
        uint32_t attachmentHeight, uint32_t attachmentLayers, bool fullscreen);
    void createSyncObjects();
    void destroySyncObjects();
    void recreateSwapchain();

    std::optional<vk::CommandBuffer*> prepareCommand();
    void finishCommand(vk::CommandBuffer* cmd);
    void excuteCompiledPass(CompiledRenderPass& compiledRenderPass, vk::CommandBuffer* cmd);

private:
    RHI::VulkanDevice* m_device;
    RenderDependencyGraph m_dependencyGraph;
    std::vector<std::unique_ptr<RenderGraphPassNode>> m_passNodes;
    std::vector<std::unique_ptr<RenderGraphResourceNode>> m_resourceNodes;
    std::vector<std::unique_ptr<IRenderGraphPassExecutor>> m_executors;
    std::vector<std::unique_ptr<RenderGraphVirtualResourceBase>> m_virtualResources;
    std::vector<std::unique_ptr<RenderGraphResourceBase>> m_resources;

    RenderGraphHandle<RHI::VulkanImageResource> m_presentHandle{RenderGraphHandleBase::InValid};

    std::vector<CompiledRenderPass> m_compiledRenderPasses;
    VkSyncObjects m_syncObjects;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_imageIdx;
    bool m_frameBufferSizeChanged = false;
};

} // namespace Render