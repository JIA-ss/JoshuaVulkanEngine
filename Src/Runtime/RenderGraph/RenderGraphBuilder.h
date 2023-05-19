#pragma once
#include "Runtime/RenderGraph/RenderGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResource.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/RenderGraph/RenderGraphRegistry.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"

namespace Render
{

class RenderGraphPassNode;
class RenderGraphBuilder
{
public:
    using PassSetUpCallback =
        std::function<IRenderGraphPassExecutor::ExecuteCallback && (RenderGraphBuilder&, RenderGraphRegistry&)>;

    RenderGraphBuilder(RenderGraph* renderGraph, RenderGraphPassNode* passNode = nullptr)
        : m_renderGraph(renderGraph), m_currentPassNode(passNode)
    {}
    ~RenderGraphBuilder() = default;

    RenderGraphPassNode* addPassNode(const char* name, PassSetUpCallback&& callback);

    void addMeshPass();

    void setResourceDescriptor(RenderGraphHandleBase handle, RenderGraphVirtualResourceBase::Description desc);

#pragma region == create image resource ==
    RenderGraphHandle<RHI::VulkanImageResource> createImageResourceNode(
        const char* name, RHI::VulkanDevice* device, vk::MemoryPropertyFlags memProps,
        RHI::VulkanImageResource::Config config);

    RenderGraphHandle<RHI::VulkanImageResource>
    createImageResourceNode(const char* name, RHI::VulkanDevice* device, RHI::VulkanImageResource::Native native);

    RenderGraphHandle<RHI::VulkanImageResource>
    createImageResourceNode(const char* name, std::unique_ptr<RHI::VulkanImageResource> resource);

#pragma endregion

#pragma region == create sampler resource ==
    RenderGraphHandle<RHI::VulkanImageSampler> createSamplerResourceNode(
        const char* name, RHI::VulkanDevice* device, std::shared_ptr<Util::Texture::RawData> rawData,
        vk::MemoryPropertyFlags memProps, RHI::VulkanImageSampler::Config config,
        RHI::VulkanImageResource::Config resourceConfig);

    RenderGraphHandle<RHI::VulkanImageSampler>
    createSamplerResourceNode(const char* name, std::unique_ptr<RHI::VulkanImageSampler> resource);
#pragma endregion

#pragma region == create buffer resource ==
    RenderGraphHandle<RHI::VulkanBuffer> createBufferResourceNode(
        const char* name, vk::DescriptorType type, RHI::VulkanDevice* device, vk::DeviceSize size,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode);

    RenderGraphHandle<RHI::VulkanBuffer>
    createBufferResourceNode(const char* name, vk::DescriptorType type, std::unique_ptr<RHI::VulkanBuffer> buffer);

    RenderGraphHandle<RHI::VulkanGPUBuffer> createGPUBufferResourceNode(
        const char* name, vk::DescriptorType type, RHI::VulkanDevice* device, vk::DeviceSize size,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode);

    RenderGraphHandle<RHI::VulkanGPUBuffer> createGPUBufferResourceNode(
        const char* name, vk::DescriptorType type, std::unique_ptr<RHI::VulkanGPUBuffer> buffer);

    RenderGraphHandle<RHI::VulkanVertexBuffer> createVertexBufferResourceNode(
        const char* name, RHI::VulkanDevice* device, vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags props, vk::SharingMode sharingMode = vk::SharingMode::eExclusive);

    RenderGraphHandle<RHI::VulkanVertexBuffer> createVertexBufferResourceNode(
        const char* name, RHI::VulkanDevice* device, const std::vector<RHI::VulkanVertexBuffer::DataType>& data,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props,
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive);

    RenderGraphHandle<RHI::VulkanVertexBuffer>
    createVertexBufferResourceNode(const char* name, std::unique_ptr<RHI::VulkanVertexBuffer> buffer);

    RenderGraphHandle<RHI::VulkanVertexIndexBuffer> createVertexIndexBufferResourceNode(
        const char* name, RHI::VulkanDevice* device, vk::DeviceSize size, vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags props, vk::SharingMode sharingMode = vk::SharingMode::eExclusive);

    RenderGraphHandle<RHI::VulkanVertexIndexBuffer> createVertexIndexBufferResourceNode(
        const char* name, RHI::VulkanDevice* device, const std::vector<RHI::VulkanVertexIndexBuffer::DataType>& data,
        vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props,
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive);

    RenderGraphHandle<RHI::VulkanVertexIndexBuffer>
    createVertexIndexBufferResourceNode(const char* name, std::unique_ptr<RHI::VulkanVertexIndexBuffer> buffer);

#pragma endregion

    template <typename ResourceType> void read(RenderGraphHandle<ResourceType> input, uint64_t flags)
    {
        assert(m_currentPassNode);
        RenderGraphRegistry registry(m_renderGraph, m_currentPassNode);
        registry.registerPassReadingDependency(m_currentPassNode, input);
    }

    template <typename ResourceType> void write(RenderGraphHandle<ResourceType> output, uint64_t flags)
    {
        assert(m_currentPassNode);
        RenderGraphRegistry registry(m_renderGraph, m_currentPassNode);
        registry.registerPassWritingDependency(m_currentPassNode, output);
    }

    void sideEffect();

    inline RenderGraphPassNode* getCurrentPassNode() const { return m_currentPassNode; }

private:
    RenderGraph* m_renderGraph = nullptr;
    RenderGraphPassNode* m_currentPassNode = nullptr;
};

} // namespace Render