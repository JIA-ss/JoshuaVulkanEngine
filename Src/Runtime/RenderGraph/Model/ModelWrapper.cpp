#include "ModelWrapper.h"
#include "Runtime/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/RenderGraph/RenderGraphRegistry.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "vulkan/vulkan_enums.hpp"
using namespace Render;

ModelWrapper::ModelWrapper(RHI::VulkanDevice* device, const boost::filesystem::path& modelPath)
{
    m_name = modelPath.filename().string();
    m_modelData = std::move(Util::Model::AssimpObj(modelPath).MoveModelData());
}

void ModelWrapper::registerTextures(RenderGraphBuilder& builder, RenderGraphRegistry& registry)
{
    for (auto& matData : m_modelData.materialDatas)
    {
        for (auto& texData : matData.textureDatas)
        {
            if (m_samplerHandles.find(texData.name) != m_samplerHandles.end())
            {
                continue;
            }

            RHI::VulkanImageSampler::Config imageSamplerConfig;
            RHI::VulkanImageResource::Config imageResourceConfig;
            if (texData.rawData)
            {
                imageResourceConfig.extent =
                    vk::Extent3D{(uint32_t)texData.rawData->GetWidth(), (uint32_t)texData.rawData->GetHeight(), 1};
                if (texData.rawData->IsCubeMap())
                {
                    imageSamplerConfig = RHI::VulkanImageSampler::Config::CubeMap(texData.rawData->GetMipLevels());
                    imageResourceConfig = RHI::VulkanImageResource::Config::CubeMap(
                        texData.rawData->GetWidth(), texData.rawData->GetHeight(), texData.rawData->GetMipLevels());
                }
                imageResourceConfig.format = texData.rawData->GetVkFormat();
                m_samplerHandles[texData.name] = builder.createSamplerResourceNode(
                    getResourceNameInRenderGraph(texData.name).c_str(), m_device, texData.rawData,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, imageSamplerConfig, imageResourceConfig);
            }
        }
    }
}

void ModelWrapper::registerBuffers(RenderGraphBuilder& builder, RenderGraphRegistry& registry)
{
    for (int meshIdx = 0; meshIdx < m_modelData.meshDatas.size(); meshIdx++)
    {
        auto& meshData = m_modelData.meshDatas[meshIdx];
        if (m_vertexBufferHandles.find(meshData.name) != m_vertexBufferHandles.end())
        {
            continue;
        }

        // register vertex buffer
        m_vertexBufferHandles[meshData.name] = builder.createVertexBufferResourceNode(
            getResourceNameInRenderGraph(meshData.name).c_str(), m_device, meshData.vertices,
            vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // register index buffer
        m_indexBufferHandles[meshData.name] = builder.createVertexIndexBufferResourceNode(
            getResourceNameInRenderGraph(meshData.name).c_str(), m_device, meshData.indices,
            vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

        // register relationship with sampler
        auto& materialData = m_modelData.materialDatas[m_modelData.materialIndexs[meshIdx]];
        for (int texIdx = 0; texIdx < materialData.textureDatas.size(); texIdx++)
        {
            std::string texName = materialData.textureDatas[texIdx].name;

            m_vertexBufferToSamplerHandles[m_vertexBufferHandles[meshData.name]].push_back(m_samplerHandles[texName]);

            auto virtualResource = registry.getResourceNode(m_samplerHandles[texName])->getVirtualResource();
            auto desc = virtualResource->getDescription();
            desc.bindingId = texIdx + 1;
            desc.setId = 1;
            desc.type = vk::DescriptorType::eCombinedImageSampler;
            virtualResource->setDescription(desc);
        }
    }

    // register uniform buffer
    m_uniformBufferHandle = builder.createBufferResourceNode(
        getResourceNameInRenderGraph("UniformBuffer").c_str(), vk::DescriptorType::eUniformBuffer, m_device,
        sizeof(RHI::ModelUniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vk::SharingMode::eExclusive);

    auto virtualResource = registry.getResourceNode(m_uniformBufferHandle)->getVirtualResource();
    auto desc = virtualResource->getDescription();
    desc.bindingId = RHI::VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID;
    desc.setId = 0;
    desc.type = vk::DescriptorType::eUniformBuffer;
    virtualResource->setDescription(desc);
}

void ModelWrapper::registerToRenderGraph(RenderGraphBuilder& builder, RenderGraphRegistry& registry)
{
    registerTextures(builder, registry);
    registerBuffers(builder, registry);
}

void ModelWrapper::uploadBuffers(RenderGraphRegistry& registry)
{
    auto cmd = m_device->GetPVulkanCmdPool()->CreateReUsableCmd();
    for (auto& meshData : m_modelData.meshDatas)
    {
        auto vertexHandle = m_vertexBufferHandles[meshData.name];
        auto indexHandle = m_indexBufferHandles[meshData.name];
        registry.getVirtualResource(vertexHandle)
            ->getResourceRef()
            ->getInternalResource()
            ->CopyDataToGPU(
                cmd, m_device->GetVkGraphicQueue(), meshData.vertices.size() * sizeof(meshData.vertices[0]));
        cmd.reset();
        registry.getVirtualResource(indexHandle)
            ->getResourceRef()
            ->getInternalResource()
            ->CopyDataToGPU(cmd, m_device->GetVkGraphicQueue(), meshData.indices.size() * sizeof(meshData.indices[0]));
        cmd.reset();
    }
    m_device->GetPVulkanCmdPool()->FreeReUsableCmd(cmd);
}

void ModelWrapper::updateUniformBuffer(RenderGraphRegistry& registry)
{
    RHI::ModelUniformBufferObject ubo;
    ubo.model = m_transformation.GetMatrix();
    ubo.color = m_color;

    if (m_uniformBufferObject.model != ubo.model || m_uniformBufferObject.color != ubo.color)
    {
        m_uniformBufferObject = ubo;
        registry.getVirtualResource(m_uniformBufferHandle)
            ->getResourceRef()
            ->getInternalResource()
            ->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
    }
}

void ModelWrapper::readByPassNode(RenderGraphBuilder& builder, RenderGraphRegistry& registry)
{
    assert(builder.getCurrentPassNode() != nullptr);
}

std::string ModelWrapper::getResourceNameInRenderGraph(const std::string& name) const { return m_name + ": " + name; }
