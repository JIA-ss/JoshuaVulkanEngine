#include "Material.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_handles.hpp"
#include <memory>

RHI_NAMESPACE_USING

Material::Material(VulkanDevice* device, const std::vector<std::weak_ptr<VulkanDescriptorSets>>& descriptors)
    : m_pVulkanDevice(device)
    , m_pVulkanDescriptorSets(descriptors)
{

}


void Material::bind(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding)
{
    for (auto& desc : m_pVulkanDescriptorSets)
    {
        if (!desc.expired())
        {
            desc.lock()->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
        }
    }
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
}