#include "RenderGraphBuilder.h"
#include "Runtime/RenderGraph/RenderGraphRegistry.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/RenderGraph/Pass/RenderGraphPassNode.h"

using namespace Render;

RenderGraphPassNode* RenderGraphBuilder::addPassNode(const char* name, PassSetUpCallback&& callback)
{
    assert(!m_currentPassNode);
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);

    assert(!registery.getPassNode(name) && "pass name already exist");

    // create pass node
    m_renderGraph->m_passNodes.emplace_back(std::make_unique<RenderGraphPassNode>(name, m_renderGraph->m_device));
    m_currentPassNode = m_renderGraph->m_passNodes.back().get();
    registery.m_currentPassNode = m_currentPassNode;

    // register pass node
    registery.registerPassNode(m_currentPassNode);

    auto&& executor = callback(*this, registery);
    // create executor
    m_renderGraph->m_executors.emplace_back(
        std::make_unique<RenderGraphRenderPassExecutor>(m_currentPassNode, std::move(executor)));
    // set executor
    m_renderGraph->m_passNodes.back()->setExecutor(m_renderGraph->m_executors.back().get());

    return m_renderGraph->m_passNodes.back().get();
}

void RenderGraphBuilder::setResourceDescriptor(
    RenderGraphHandleBase handle, RenderGraphVirtualResourceBase::Description desc)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    registery.getResourceNode(handle)->getVirtualResource()->setDescription(desc);
}

#pragma region == create image resource ==
RenderGraphHandle<RHI::VulkanImageResource> RenderGraphBuilder::createImageResourceNode(
    const char* name, RHI::VulkanDevice* device, vk::MemoryPropertyFlags memProps,
    RHI::VulkanImageResource::Config config)
{
    auto resource = std::make_unique<RHI::VulkanImageResource>(device, memProps, config);
    return createImageResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanImageResource> RenderGraphBuilder::createImageResourceNode(
    const char* name, RHI::VulkanDevice* device, RHI::VulkanImageResource::Native native)
{
    auto resource = std::make_unique<RHI::VulkanImageResource>(device, native);
    return createImageResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanImageResource>
RenderGraphBuilder::createImageResourceNode(const char* name, std::unique_ptr<RHI::VulkanImageResource> resource)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = vk::DescriptorType::eCombinedImageSampler;
    return registery.registerResourceNode(name, desc, std::move(resource));
}

#pragma endregion

#pragma region == create sampler resource ==

RenderGraphHandle<RHI::VulkanImageSampler> RenderGraphBuilder::createSamplerResourceNode(
    const char* name, RHI::VulkanDevice* device, std::shared_ptr<Util::Texture::RawData> rawData,
    vk::MemoryPropertyFlags memProps, RHI::VulkanImageSampler::Config config,
    RHI::VulkanImageResource::Config resourceConfig)
{
    auto resource = std::make_unique<RHI::VulkanImageSampler>(device, rawData, memProps, config, resourceConfig);
    return createSamplerResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanImageSampler>
RenderGraphBuilder::createSamplerResourceNode(const char* name, std::unique_ptr<RHI::VulkanImageSampler> resource)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = vk::DescriptorType::eCombinedImageSampler;
    return registery.registerResourceNode(name, desc, std::move(resource));
}

#pragma endregion

#pragma region == create buffer resource ==

RenderGraphHandle<RHI::VulkanBuffer> RenderGraphBuilder::createBufferResourceNode(
    const char* name, vk::DescriptorType type, RHI::VulkanDevice* device, vk::DeviceSize size,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode)
{
    auto resource = std::make_unique<RHI::VulkanBuffer>(device, size, usage, properties, sharingMode);
    return createBufferResourceNode(name, type, std::move(resource));
}

RenderGraphHandle<RHI::VulkanBuffer> RenderGraphBuilder::createBufferResourceNode(
    const char* name, vk::DescriptorType type, std::unique_ptr<RHI::VulkanBuffer> buffer)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = type;
    return registery.registerResourceNode(name, desc, std::move(buffer));
}

RenderGraphHandle<RHI::VulkanGPUBuffer> RenderGraphBuilder::createGPUBufferResourceNode(
    const char* name, vk::DescriptorType type, RHI::VulkanDevice* device, vk::DeviceSize size,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
{
    auto resource = std::make_unique<RHI::VulkanGPUBuffer>(device, size, usage, props, sharingMode);
    return createGPUBufferResourceNode(name, type, std::move(resource));
}

RenderGraphHandle<RHI::VulkanGPUBuffer> RenderGraphBuilder::createGPUBufferResourceNode(
    const char* name, vk::DescriptorType type, std::unique_ptr<RHI::VulkanGPUBuffer> buffer)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = type;
    return registery.registerResourceNode(name, desc, std::move(buffer));
}

RenderGraphHandle<RHI::VulkanVertexBuffer> RenderGraphBuilder::createVertexBufferResourceNode(
    const char* name, RHI::VulkanDevice* device, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
{
    auto resource = RHI::VulkanVertexBuffer::Create(device, size, usage, props, sharingMode);
    return createVertexBufferResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanVertexBuffer> RenderGraphBuilder::createVertexBufferResourceNode(
    const char* name, RHI::VulkanDevice* device, const std::vector<RHI::VulkanVertexBuffer::DataType>& data,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
{
    auto resource = RHI::VulkanVertexBuffer::Create(device, data, usage, props, sharingMode);
    return createVertexBufferResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanVertexBuffer>
RenderGraphBuilder::createVertexBufferResourceNode(const char* name, std::unique_ptr<RHI::VulkanVertexBuffer> buffer)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = vk::DescriptorType::eInputAttachment;
    return registery.registerResourceNode(name, desc, std::move(buffer));
}

RenderGraphHandle<RHI::VulkanVertexIndexBuffer> RenderGraphBuilder::createVertexIndexBufferResourceNode(
    const char* name, RHI::VulkanDevice* device, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
{
    auto resource = RHI::VulkanVertexIndexBuffer::Create(device, size, usage, props, sharingMode);
    return createVertexIndexBufferResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanVertexIndexBuffer> RenderGraphBuilder::createVertexIndexBufferResourceNode(
    const char* name, RHI::VulkanDevice* device, const std::vector<RHI::VulkanVertexIndexBuffer::DataType>& data,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
{
    auto resource = RHI::VulkanVertexIndexBuffer::Create(device, data, usage, props, sharingMode);
    return createVertexIndexBufferResourceNode(name, std::move(resource));
}

RenderGraphHandle<RHI::VulkanVertexIndexBuffer> RenderGraphBuilder::createVertexIndexBufferResourceNode(
    const char* name, std::unique_ptr<RHI::VulkanVertexIndexBuffer> buffer)
{
    RenderGraphRegistry registery(m_renderGraph, m_currentPassNode);
    RenderGraphVirtualResourceBase::Description desc;
    desc.type = vk::DescriptorType::eInputAttachment;
    return registery.registerResourceNode(name, desc, std::move(buffer));
}

#pragma endregion