#include "Model.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/Graphic/Material.h"
#include "Runtime/VulkanRHI/Graphic/Mesh.h"
#include "Util/Fileutil.h"
#include "Util/Modelutil.h"
#include "Util/Textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <assimp/material.h>
#include <glm/ext/matrix_transform.hpp>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
RHI_NAMESPACE_USING


Model::Model(VulkanDevice* device, Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout, const glm::vec4& color)
    : m_pVulkanDevice(device)
    , m_color(color)
{
    init(std::move(modelData), layout);
}

Model::Model(VulkanDevice* device, const boost::filesystem::path& modelPath, VulkanDescriptorSetLayout* layout, const glm::vec4& color)
    : m_pVulkanDevice(device)
    , m_color(color)
{
    init(Util::Model::AssimpObj(modelPath).MoveModelData(), layout);
}

Model::~Model()
{
    m_descriptorsets.clear();
    m_materials.clear();
    m_meshes.clear();
    m_vulkanImageSamplers.clear();
    m_vulkanDescriptorPool.reset();
}

void Model::InitShadowPassUniforDescriptorSets(const std::array<std::vector<UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo, int lightIdx)
{
    constexpr const uint32_t BINDINGID = VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID;
    constexpr const uint32_t RANGE = sizeof(ModelUniformBufferObject);
    for (int frameId = 0; frameId < uboInfo.size(); frameId++)
    {
        std::vector<VulkanBuffer*> buffer;
        std::vector<uint32_t> binding, range;
        for (auto& layoutInfo : uboInfo[frameId])
        {
            buffer.push_back(layoutInfo.buffer);
            binding.push_back(layoutInfo.bindingId);
            range.push_back(layoutInfo.range);
        }

        buffer.push_back(m_uniformBuffers[frameId].get());
        binding.push_back(BINDINGID);
        range.push_back(RANGE);

        if (m_shadowPassUniformSets[frameId].size() <= lightIdx)
        {
            m_shadowPassUniformSets[frameId].resize(lightIdx + 1);
        }

        m_shadowPassUniformSets[frameId][lightIdx] = m_vulkanDescriptorPool->AllocUniformDescriptorSet(m_pVulkanDevice->GetDescLayoutPresets().UBO.get(), buffer, binding, range, 1);
    }
}

void Model::InitUniformDescriptorSets(const std::array<std::vector<UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo)
{
    constexpr const uint32_t BINDINGID = VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID;
    constexpr const uint32_t RANGE = sizeof(ModelUniformBufferObject);
    for (int frameId = 0; frameId < uboInfo.size(); frameId++)
    {
        std::vector<VulkanBuffer*> buffer;
        std::vector<uint32_t> binding, range;
        for (auto& layoutInfo : uboInfo[frameId])
        {
            uint32_t bindingId = layoutInfo.bindingId;
            if (buffer.size() <= bindingId)
            {
                buffer.resize(bindingId + 1);
                binding.resize(bindingId + 1);
                range.resize(bindingId + 1);
            }

            buffer[bindingId] = layoutInfo.buffer;
            binding[bindingId] = layoutInfo.bindingId;
            range[bindingId] = layoutInfo.range;
        }

        if (buffer.size() <= BINDINGID)
        {
            buffer.resize(BINDINGID + 1);
            binding.resize(BINDINGID + 1);
            range.resize(BINDINGID + 1);
        }
        buffer[BINDINGID] = m_uniformBuffers[frameId].get();
        binding[BINDINGID] = BINDINGID;
        range[BINDINGID] = RANGE;
        m_uniformSets[frameId] = m_vulkanDescriptorPool->AllocUniformDescriptorSet(m_pVulkanDevice->GetDescLayoutPresets().UBO.get(), buffer, binding, range, 1);
    }
}

void Model::DrawShadowPass(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, int frameId, int lightId)
{
    // bind shadowpass descriptor
    {
        updateModelUniformBuffer(frameId);
        std::vector<vk::DescriptorSet> CAMUBO_Descriptors;
        m_shadowPassUniformSets[frameId][lightId]->FillToBindedDescriptorSetsVector(CAMUBO_Descriptors, pipelineLayout);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->GetVkPieplineLayout(), 0, CAMUBO_Descriptors, {});
    }

    for (auto& mesh : m_meshes)
    {
        mesh->Bind(cmd);
        mesh->DrawIndexed(cmd);
    }
}


void Model::DrawWithNoMaterial(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId)
{
    // bind model ubo
    {
        updateModelUniformBuffer(frameId);
        m_uniformSets[frameId]->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
    }

    for (auto& mesh : m_meshes)
    {
        mesh->Bind(cmd);
        mesh->DrawIndexed(cmd);
    }
}

void Model::Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId)
{
    // bind model ubo
    {
        updateModelUniformBuffer(0);
        m_uniformSets[0]->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
    }

    for (int meshIdx = 0; meshIdx < m_meshes.size(); meshIdx++)
    {
        m_meshes[meshIdx]->Bind(cmd);
        int matIdx = m_materialIndexs[meshIdx];
        if (matIdx >= 0 && matIdx < m_materials.size() && m_materials[matIdx])
        {
            m_materials[matIdx]->bind(cmd, pipelineLayout, tobinding);
        }
        m_meshes[meshIdx]->DrawIndexed(cmd);
    }
}

void Model::init(Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout)
{
    m_materialIndexs = std::move(modelData.materialIndexs);

    initMatrials(modelData.materialDatas, layout);
    initMeshes(modelData.meshDatas);
    initModelUniformBuffers();
    m_materialData = modelData.materialDatas;
}
void Model::initMatrials(const std::vector<Util::Model::MaterialData>& materialDatas, VulkanDescriptorSetLayout* layout)
{
    // create image sampler
    for (auto& matData : materialDatas)
    {
        if (m_descriptorsets.find(matData.name) != m_descriptorsets.end())
        {
            continue;
        }
        for (auto& texData : matData.textureDatas)
        {
            if (m_vulkanImageSamplers.find(texData.name) == m_vulkanImageSamplers.end())
            {
                VulkanImageSampler::Config imageSamplerConfig;
                VulkanImageResource::Config imageResourceConfig;
                if (texData.rawData)
                {
                    imageResourceConfig.extent = vk::Extent3D{(uint32_t)texData.rawData->GetWidth(), (uint32_t)texData.rawData->GetHeight(), 1};
                    if (texData.rawData->IsCubeMap())
                    {
                        imageSamplerConfig = VulkanImageSampler::Config::CubeMap(texData.rawData->GetMipLevels());
                        imageResourceConfig = VulkanImageResource::Config::CubeMap(texData.rawData->GetWidth(), texData.rawData->GetHeight(), texData.rawData->GetMipLevels());
                    }
                    imageResourceConfig.format = texData.rawData->GetVkFormat();
                    m_vulkanImageSamplers[texData.name] = std::make_shared<VulkanImageSampler>(
                        m_pVulkanDevice,
                        texData.rawData,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        imageSamplerConfig,
                        imageResourceConfig
                    );
                }
            }
        }
        m_descriptorsets[matData.name] = nullptr;
    }

    // create descriptorsets pool
    uint32_t maxDescriptorNums = 0;
    uint32_t minSamplerBindingSlot = VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER1_BINDING_ID;
    uint32_t maxSamplerBindingSlot = VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER5_BINDING_ID;
    for (uint32_t i = minSamplerBindingSlot; i <= maxSamplerBindingSlot; i++)
    {
        maxDescriptorNums += m_vulkanImageSamplers.size() / i;
    }


    if (maxDescriptorNums > 0)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes
        {
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, maxDescriptorNums },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, (uint32_t)m_uniformBuffers.size() + 5 * (uint32_t)m_shadowPassUniformSets.size() }
        };
        m_vulkanDescriptorPool.reset(new VulkanDescriptorPool(m_pVulkanDevice, poolSizes, poolSizes[0].descriptorCount + poolSizes[1].descriptorCount));
    }

    // create descriptorsets
    std::map<std::string, std::shared_ptr<VulkanDescriptorSets>> vulkanDescSets;
    std::map<std::shared_ptr<VulkanImageSampler>, std::shared_ptr<VulkanDescriptorSets>> samplerToDescSet;
    for (auto& matData : materialDatas)
    {
        std::vector<VulkanImageSampler*> samplers;
        if (m_descriptorsets[matData.name] != nullptr)
        {
            continue;
            // mat.reset(new Material(m_pVulkanDevice, {m_descriptorsets[matData.name]}));
        }
        if (matData.textureDatas.size() > 0)
        {
            for (auto& texData : matData.textureDatas)
            {
                samplers.emplace_back(m_vulkanImageSamplers[texData.name].get());
            }

            std::vector<uint32_t> binding;
            for (int i = minSamplerBindingSlot; i <= maxSamplerBindingSlot && i <= samplers.size(); i++)
            {
                binding.emplace_back(i);
            }
            std::shared_ptr<VulkanDescriptorSets> descs = m_vulkanDescriptorPool->AllocSamplerDescriptorSet(layout, samplers, binding);
            m_descriptorsets[matData.name] = descs;
        }
        else
        {
            auto shaderDatas = matData.shaderDatas;
            auto texDatas = matData.textureDatas;
            auto matName = matData.name;
            // assert(false);
        }
    }

    std::map<std::string, int> matNameToIdx;
    m_materials.resize(materialDatas.size());
    for (int i = 0; i < materialDatas.size(); i++)
    {
        auto& matData = materialDatas[i];
        if (matNameToIdx.find(matData.name) != matNameToIdx.end())
        {
            m_materials[i] = m_materials[matNameToIdx[matData.name]];
            continue;
        }

        m_materials[i].reset(new Material(m_pVulkanDevice, {m_descriptorsets[matData.name]}));
    }
}

void Model::initMeshes(std::vector<Util::Model::MeshData>& meshData)
{
    for (int i = 0; i < meshData.size(); i++)
    {
        std::shared_ptr<Mesh> mesh(new Mesh(m_pVulkanDevice, std::move(meshData[i])));
        m_meshes.emplace_back(mesh);
    }
}

void Model::initModelUniformBuffers()
{
    for (int i = 0; i < m_uniformBuffers.size(); i++)
    {
        m_uniformBuffers[i].reset(
            new RHI::VulkanBuffer(
                    m_pVulkanDevice, sizeof(ModelUniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    vk::SharingMode::eExclusive
                )
            );
    }
}

void Model::updateModelUniformBuffer(int frameId)
{
    ModelUniformBufferObject ubo;
    ubo.model = m_transformation.GetMatrix();
    ubo.color = m_color;

    if (m_uniformBufferObjects[frameId].model != ubo.model
        || m_uniformBufferObjects[frameId].color  != ubo.color)
    {
        m_uniformBufferObjects[frameId] = ubo;
        m_uniformBuffers[frameId]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
    }
}
#pragma region === Model View For Debug ===


ModelView::ModelView(Model* model, VulkanDevice* device, VulkanDescriptorSetLayout* layout)
    : m_model(model)
    , m_pVulkanDevice(device)
{
    m_meshViews.resize(model->m_meshes.size());
    for (int i = 0; i < m_meshViews.size(); i++)
    {
        m_meshViews[i].reset(new MeshView(model->m_meshes[i].get(), device));
    }


    // create image sampler
    for (auto& matData : model->m_materialData)
    {
        if (m_descriptorsets.find(matData.name) != m_descriptorsets.end())
        {
            continue;
        }
        for (auto& texData : matData.textureDatas)
        {
            if (m_vulkanImageSamplers.find(texData.name) == m_vulkanImageSamplers.end())
            {
                VulkanImageSampler::Config imageSamplerConfig;
                VulkanImageResource::Config imageResourceConfig;
                if (texData.rawData)
                {
                    imageResourceConfig.extent = vk::Extent3D{(uint32_t)texData.rawData->GetWidth(), (uint32_t)texData.rawData->GetHeight(), 1};
                    m_vulkanImageSamplers[texData.name] = std::make_shared<VulkanImageSampler>(
                        m_pVulkanDevice,
                        texData.rawData,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        imageSamplerConfig,
                        imageResourceConfig
                    );
                }
            }
        }
        m_descriptorsets[matData.name] = nullptr;
    }

    // create descriptorsets pool
    uint32_t maxDescriptorNums = 0;
    uint32_t minSamplerBindingSlot = VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER1_BINDING_ID;
    uint32_t maxSamplerBindingSlot = VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER5_BINDING_ID;
    for (uint32_t i = minSamplerBindingSlot; i <= maxSamplerBindingSlot; i++)
    {
        maxDescriptorNums += m_vulkanImageSamplers.size() / i;
    }


    if (maxDescriptorNums > 0)
    {
        std::vector<vk::DescriptorPoolSize> poolSizes
        {
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, maxDescriptorNums },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, (uint32_t)m_uniformBuffers.size()}
        };
        m_vulkanDescriptorPool.reset(new VulkanDescriptorPool(m_pVulkanDevice, poolSizes, poolSizes[0].descriptorCount + poolSizes[1].descriptorCount));
    }

    // create descriptorsets
    std::map<std::string, std::shared_ptr<VulkanDescriptorSets>> vulkanDescSets;
    std::map<std::shared_ptr<VulkanImageSampler>, std::shared_ptr<VulkanDescriptorSets>> samplerToDescSet;
    for (auto& matData : m_model->m_materialData)
    {
        std::vector<VulkanImageSampler*> samplers;
        if (m_descriptorsets[matData.name] != nullptr)
        {
            continue;
            // mat.reset(new Material(m_pVulkanDevice, {m_descriptorsets[matData.name]}));
        }
        if (matData.textureDatas.size() > 0)
        {
            for (auto& texData : matData.textureDatas)
            {
                samplers.emplace_back(m_vulkanImageSamplers[texData.name].get());
            }

            std::vector<uint32_t> binding;
            for (int i = minSamplerBindingSlot; i <= maxSamplerBindingSlot && i <= samplers.size(); i++)
            {
                binding.emplace_back(i);
            }
            std::shared_ptr<VulkanDescriptorSets> descs = m_vulkanDescriptorPool->AllocSamplerDescriptorSet(layout, samplers, binding);
            m_descriptorsets[matData.name] = descs;
        }
        else
        {
            auto shaderDatas = matData.shaderDatas;
            auto texDatas = matData.textureDatas;
            auto matName = matData.name;
            // assert(false);
        }
    }

    std::map<std::string, int> matNameToIdx;
    m_materials.resize(m_model->m_materialData.size());
    for (int i = 0; i < m_materials.size(); i++)
    {
        auto& matData = m_model->m_materialData[i];
        if (matNameToIdx.find(matData.name) != matNameToIdx.end())
        {
            m_materials[i] = m_materials[matNameToIdx[matData.name]];
            continue;
        }

        m_materials[i].reset(new Material(m_pVulkanDevice, {m_descriptorsets[matData.name]}));
    }

    // init uniform buffers
    for (int i = 0; i < m_uniformBuffers.size(); i++)
    {
        m_uniformBuffers[i].reset(
            new RHI::VulkanBuffer(
                    m_pVulkanDevice, sizeof(ModelUniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    vk::SharingMode::eExclusive
                )
            );
    }
}

ModelView::~ModelView()
{

}

void ModelView::InitUniformDescriptorSets(const std::array<std::vector<Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo)
{
    constexpr const uint32_t BINDINGID = VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID;
    constexpr const uint32_t RANGE = sizeof(ModelUniformBufferObject);
    for (int frameId = 0; frameId < uboInfo.size(); frameId++)
    {
        std::vector<VulkanBuffer*> buffer;
        std::vector<uint32_t> binding, range;
        for (auto& layoutInfo : uboInfo[frameId])
        {
            uint32_t bindingId = layoutInfo.bindingId;
            if (buffer.size() <= bindingId)
            {
                buffer.resize(bindingId + 1);
                binding.resize(bindingId + 1);
                range.resize(bindingId + 1);
            }

            buffer[bindingId] = layoutInfo.buffer;
            binding[bindingId] = layoutInfo.bindingId;
            range[bindingId] = layoutInfo.range;
        }

        if (buffer.size() <= BINDINGID)
        {
            buffer.resize(BINDINGID + 1);
            binding.resize(BINDINGID + 1);
            range.resize(BINDINGID + 1);
        }
        buffer[BINDINGID] = m_uniformBuffers[frameId].get();
        binding[BINDINGID] = BINDINGID;
        range[BINDINGID] = RANGE;
        m_uniformSets[frameId] = m_vulkanDescriptorPool->AllocUniformDescriptorSet(m_pVulkanDevice->GetDescLayoutPresets().UBO.get(), buffer, binding, range, 1);
    }
}

void ModelView::DrawWithNoMaterial(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId)
{
    // bind model ubo
    {
        updateModelUniformBuffer(frameId);
        m_uniformSets[frameId]->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
    }

    for (auto& meshView : m_meshViews)
    {
        meshView->Bind(cmd);
        meshView->DrawIndexed(cmd);
    }
}
void ModelView::Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId)
{
    // bind model ubo
    {
        updateModelUniformBuffer(frameId);
        m_uniformSets[frameId]->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
    }

    for (int meshIdx = 0; meshIdx < m_meshViews.size(); meshIdx++)
    {
        m_meshViews[meshIdx]->Bind(cmd);
        int matIdx = m_model->m_materialIndexs[meshIdx];
        if (matIdx >= 0 && matIdx < m_materials.size() && m_materials[matIdx])
        {
            m_materials[matIdx]->bind(cmd, pipelineLayout, tobinding);
        }
        m_meshViews[meshIdx]->DrawIndexed(cmd);
    }
}

Util::Math::SRTMatrix& ModelView::GetTransformation()
{
    return m_model->m_transformation;
}


void ModelView::updateModelUniformBuffer(int frameId)
{
    ModelUniformBufferObject ubo;
    ubo.model = m_model->m_transformation.GetMatrix();
    ubo.color = m_model->m_color;

    if (m_uniformBufferObjects[frameId].model != ubo.model
        || m_uniformBufferObjects[frameId].color != ubo.color)
    {
        m_uniformBufferObjects[frameId] = ubo;
        m_uniformBuffers[frameId]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
    }
}
#pragma endregion