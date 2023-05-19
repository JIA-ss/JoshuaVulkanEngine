#pragma once
#include "Runtime/RenderGraph/Pass/RenderGraphPassExecutor.h"
#include "Runtime/RenderGraph/RenderDependencyGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include <stdint.h>
#include <vector>
namespace Render
{

class RenderGraphBuilder;
class RenderGraphPassNode : public RenderDependencyGraph::Node
{
    friend class RenderGraphBuilder;

public:
    struct AttachmentMeta
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t layer = 0;
        bool fullScreen = true;
        std::vector<vk::ClearValue> clearValues;

        bool operator==(const AttachmentMeta& other) const
        {
            if (fullScreen && other.fullScreen)
            {
                if (clearValues.size() != other.clearValues.size())
                {
                    return false;
                }
                for (int i = 0; i < clearValues.size(); ++i)
                {
                    if (clearValues[i].color.float32 == other.clearValues[i].color.float32 &&
                        clearValues[i].color.int32 == other.clearValues[i].color.int32 &&
                        clearValues[i].color.uint32 == other.clearValues[i].color.uint32 &&
                        clearValues[i].depthStencil.depth == other.clearValues[i].depthStencil.depth &&
                        clearValues[i].depthStencil.stencil == other.clearValues[i].depthStencil.stencil)
                    {
                        return true;
                    }
                }
                return false;
            }
            if (width == other.width && height == other.height && layer == other.layer)
            {
                for (int i = 0; i < clearValues.size(); ++i)
                {
                    if (clearValues[i].color.float32 == other.clearValues[i].color.float32 &&
                        clearValues[i].color.int32 == other.clearValues[i].color.int32 &&
                        clearValues[i].color.uint32 == other.clearValues[i].color.uint32 &&
                        clearValues[i].depthStencil.depth == other.clearValues[i].depthStencil.depth &&
                        clearValues[i].depthStencil.stencil == other.clearValues[i].depthStencil.stencil)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        bool operator!=(const AttachmentMeta& other) const { return !(*this == other); }
    };

    struct MeshPassData
    {
        RenderGraphHandle<RHI::VulkanVertexBuffer> vertexBuffer;
        RenderGraphHandle<RHI::VulkanVertexIndexBuffer> indexBuffer;
        std::vector<std::vector<RenderGraphHandleBase>> descriptorSets;
    };

public:
    explicit RenderGraphPassNode(const char* name, RHI::VulkanDevice* device)
        : m_name(name), m_pipelineBuilder(device, nullptr)
    {}
    inline const char* getName() const { return m_name; }

#pragma region == binding info ==
    inline void setBindingInfo(const RHI::VulkanPipelineLayout::BindingInfo& bindingInfo)
    {
        m_bindingInfo = bindingInfo;
    }
    const RHI::VulkanPipelineLayout::BindingInfo& getBindingInfo() const { return m_bindingInfo; }
#pragma endregion

#pragma region == attachment info ==
    inline void setAttachments(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
    {
        m_attachments = attachments;
    }
    const std::vector<RHI::VulkanFramebuffer::Attachment>& getAttachments() const { return m_attachments; }

    inline void setAttachmentMeta(const AttachmentMeta& attachmentMeta) { m_attachmentMeta = attachmentMeta; }
    const AttachmentMeta& getAttachmentMeta() const { return m_attachmentMeta; }
#pragma endregion

#pragma region == subpass info ==
    inline void setSubpassDependencies(const std::array<vk::SubpassDependency, 2>& subpassDependencies)
    {
        m_subpassDependencies = subpassDependencies;
    }
    const std::array<vk::SubpassDependency, 2>& getSubpassDependencies() const { return m_subpassDependencies; }

    inline void setSubpassDescription(const vk::SubpassDescription& subpassDescription)
    {
        m_subpassDescription = subpassDescription;
    }
    const vk::SubpassDescription& getSubpassDescription() const { return m_subpassDescription; }
#pragma endregion

#pragma region == mesh pass data ==
    inline void setMeshPassDatas(const std::vector<MeshPassData>& meshPassDatas) { m_meshPassDatas = meshPassDatas; }
    inline void addMeshPassData(const MeshPassData& meshPassData) { m_meshPassDatas.push_back(meshPassData); }
    const std::vector<MeshPassData>& getMeshPassDatas() const { return m_meshPassDatas; }
#pragma endregion

    inline RHI::VulkanRenderPipelineBuilder& getPipelineBuilder() { return m_pipelineBuilder; }

protected:
    inline void setExecutor(IRenderGraphPassExecutor* executor) { m_executor = executor; }
    inline IRenderGraphPassExecutor* getExecutor() const { return m_executor; }

protected:
    const char* m_name = nullptr;
    IRenderGraphPassExecutor* m_executor = nullptr;
    RHI::VulkanRenderPipelineBuilder m_pipelineBuilder;
    RHI::VulkanPipelineLayout::BindingInfo m_bindingInfo;
    vk::SubpassDescription m_subpassDescription;
    std::array<vk::SubpassDependency, 2> m_subpassDependencies;
    std::vector<RHI::VulkanFramebuffer::Attachment> m_attachments;
    std::vector<MeshPassData> m_meshPassDatas;
    AttachmentMeta m_attachmentMeta;
};

} // namespace Render