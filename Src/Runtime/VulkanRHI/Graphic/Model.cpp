#include "Model.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/Graphic/Material.h"
#include "Runtime/VulkanRHI/Graphic/Mesh.h"
#include "Util/Modelutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
RHI_NAMESPACE_USING


Model::Model(VulkanDevice* device, Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout)
    : m_pVulkanDevice(device)
{
    init(std::move(modelData), layout);
}

Model::Model(VulkanDevice* device, const boost::filesystem::path& modelPath, VulkanDescriptorSetLayout* layout)
    : m_pVulkanDevice(device)
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

void Model::DrawWithNoMaterial(vk::CommandBuffer& cmd)
{
    for (auto& mesh : m_meshes)
    {
        mesh->bind(cmd);
        mesh->drawIndexed(cmd);
    }
}

void Model::Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding)
{
    for (int meshIdx = 0; meshIdx < m_meshes.size(); meshIdx++)
    {
        m_meshes[meshIdx]->bind(cmd);
        int matIdx = m_materialIndexs[meshIdx];
        if (matIdx >= 0 && matIdx < m_materials.size() && m_materials[matIdx])
        {
            m_materials[matIdx]->bind(cmd, pipelineLayout, tobinding);
        }
        m_meshes[meshIdx]->drawIndexed(cmd);
    }
}

void Model::init(Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout)
{
    m_materialIndexs = std::move(modelData.materialIndexs);
    initMatrials(modelData.materialDatas, layout);
    initMeshes(modelData.meshDatas);
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
            vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, maxDescriptorNums }
        };
        m_vulkanDescriptorPool.reset(new VulkanDescriptorPool(m_pVulkanDevice, poolSizes, maxDescriptorNums));
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