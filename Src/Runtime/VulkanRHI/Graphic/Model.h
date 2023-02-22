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
#include "Util/Mathutil.h"
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
    std::vector<std::shared_ptr<VulkanDescriptorSets>> m_descriptorsets;

    Util::Math::SRTMatrix m_transformation;
public:
    explicit Model(VulkanDevice* device, Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout);
    explicit Model(VulkanDevice* device, const boost::filesystem::path& modelPath, VulkanDescriptorSetLayout* layout);
    ~Model();

    void DrawWithNoMaterial(vk::CommandBuffer& cmd);
    void Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding);

    inline Util::Math::SRTMatrix& GetTransformation() { return m_transformation; }
private:
    void init(Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout);
    void initMatrials(const std::vector<Util::Model::MaterialData>& materialData, VulkanDescriptorSetLayout* layout);
    void initMeshes(std::vector<Util::Model::MeshData>& meshData);
};

RHI_NAMESPACE_END