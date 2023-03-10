#pragma once
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Modelutil.h"
#include "vulkan/vulkan_enums.hpp"
#include <boost/filesystem/path.hpp>

RHI_NAMESPACE_BEGIN

class MaterialView;
class Material
{
    friend class Model;
    friend class ModelView;
    friend class MaterialView;
private:
    VulkanDevice* m_pVulkanDevice;
    Util::Model::MaterialData m_materialData;

    std::vector<std::weak_ptr<VulkanDescriptorSets>> m_pVulkanDescriptorSets;

public:
    explicit Material(VulkanDevice* device, const std::vector<std::weak_ptr<VulkanDescriptorSets>>& descriptors);


private:
    void bind(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding);
};

RHI_NAMESPACE_END