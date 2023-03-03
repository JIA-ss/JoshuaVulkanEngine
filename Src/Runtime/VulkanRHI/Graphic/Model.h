#pragma once
#include "Runtime/VulkanRHI/Graphic/Material.h"
#include "Runtime/VulkanRHI/Graphic/Mesh.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
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

class ModelView;
class Model
{
    friend class ModelView;
public:
    struct UBOLayoutInfo
    {
        VulkanBuffer* buffer;
        uint32_t bindingId;
        uint32_t range;
    };
private:
    VulkanDevice* m_pVulkanDevice;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Material>> m_materials;
    std::vector<size_t> m_materialIndexs;
    std::vector<Util::Model::MaterialData> m_materialData;

    std::unordered_map<std::string, std::shared_ptr<VulkanImageSampler>> m_vulkanImageSamplers;
    std::unordered_map<std::string, std::shared_ptr<VulkanDescriptorSets>> m_descriptorsets;
    std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;

    std::array<ModelUniformBufferObject, MAX_FRAMES_IN_FLIGHT> m_uniformBufferObjects;
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
    std::array<std::shared_ptr<RHI::VulkanDescriptorSets>, MAX_FRAMES_IN_FLIGHT> m_uniformSets;
    std::array<std::vector<std::shared_ptr<RHI::VulkanDescriptorSets>>, MAX_FRAMES_IN_FLIGHT> m_shadowPassUniformSets;
    Util::Math::SRTMatrix m_transformation;
    glm::vec4 m_color;
public:
    explicit Model(VulkanDevice* device, Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout, const glm::vec4& color = glm::vec4(1.0f));
    explicit Model(VulkanDevice* device, const boost::filesystem::path& modelPath, VulkanDescriptorSetLayout* layout, const glm::vec4& color = glm::vec4(1.0f));
    ~Model();

    void SetColor(const glm::vec4& color) { m_color = color; }
    void InitShadowPassUniforDescriptorSets(const std::array<std::vector<UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo, int lightIdx);
    void InitUniformDescriptorSets(const std::array<std::vector<UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo);
    void DrawShadowPass(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, int frameId, int lightId);
    void DrawWithNoMaterial(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId);
    void Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId);

    inline Util::Math::SRTMatrix& GetTransformation() { return m_transformation; }
private:
    void init(Util::Model::ModelData&& modelData, VulkanDescriptorSetLayout* layout);
    void initMatrials(const std::vector<Util::Model::MaterialData>& materialData, VulkanDescriptorSetLayout* layout);
    void initMeshes(std::vector<Util::Model::MeshData>& meshData);
    void initModelUniformBuffers();
    void updateModelUniformBuffer(int frameId);
};

class ModelView
{
private:
    VulkanDevice* m_pVulkanDevice;
    Model* m_model;
    std::vector<std::shared_ptr<MeshView>> m_meshViews;
    std::vector<std::shared_ptr<Material>> m_materials;

    std::unordered_map<std::string, std::shared_ptr<VulkanImageSampler>> m_vulkanImageSamplers;
    std::unordered_map<std::string, std::shared_ptr<VulkanDescriptorSets>> m_descriptorsets;
    std::unique_ptr<VulkanDescriptorPool> m_vulkanDescriptorPool;

    std::array<ModelUniformBufferObject, MAX_FRAMES_IN_FLIGHT> m_uniformBufferObjects;
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
    std::array<std::shared_ptr<RHI::VulkanDescriptorSets>, MAX_FRAMES_IN_FLIGHT> m_uniformSets;
public:
    explicit ModelView(Model* model, VulkanDevice* device, VulkanDescriptorSetLayout* layout);
    ~ModelView();

    void InitUniformDescriptorSets(const std::array<std::vector<Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT>& uboInfo);
    void DrawWithNoMaterial(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId);
    void Draw(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding, int frameId);
    void updateModelUniformBuffer(int frameId);
    Util::Math::SRTMatrix& GetTransformation();
};

RHI_NAMESPACE_END