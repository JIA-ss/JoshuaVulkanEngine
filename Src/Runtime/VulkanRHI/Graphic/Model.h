#pragma once
#include "Runtime/VulkanRHI/Graphic/Material.h"
#include "Runtime/VulkanRHI/Graphic/Mesh.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Modelutil.h"
#include "vulkan/vulkan_handles.hpp"
#include <boost/filesystem/path.hpp>
#include <stdint.h>
#include <unordered_map>

RHI_NAMESPACE_BEGIN

class Model
{
private:
    VulkanDevice* m_pVulkanDevice;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<size_t> m_materialIndexs;

    std::unordered_map<std::string, std::shared_ptr<VulkanImageSampler>> m_vulkanImageSamplers;
    std::unordered_map<std::string, std::shared_ptr<VulkanShaderSet>> m_vulkanShaderSets;

    std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;

    std::vector<vk::DescriptorSetLayout> m_requiredLayouts;
public:
    explicit Model(VulkanDevice* device, Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout);
    explicit Model(VulkanDevice* device, const boost::filesystem::path& modelPath, VulkanDescriptorSetLayout* layout);

    void DrawWithNoMaterial(vk::CommandBuffer& cmd);
    void Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding);
    std::vector<vk::DescriptorSetLayout> GetRequiredVkDescriptorSetLayouts();
private:
    void init(Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout);
    void initMatrials(const std::vector<Util::Model::MaterialData>& materialData, VulkanDescriptorSetLayout* layout);
    void initMeshes(std::vector<Util::Model::MeshData>& meshData);
};

RHI_NAMESPACE_END