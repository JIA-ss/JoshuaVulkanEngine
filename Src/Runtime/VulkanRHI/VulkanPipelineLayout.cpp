#include "VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSetLayout.h"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, VulkanDescriptorSetLayout* descLayout)
    : m_vulkanDevice(device)
    , m_vulkanDescSetLayout(descLayout)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts(m_vulkanDescSetLayout->GetVkDescriptorSetLayout())
                            .setPushConstantRanges(m_vkPushConstRanges);

    m_vkPipelineLayout = m_vulkanDevice->GetVkDevice().createPipelineLayout(pipelineLayoutCreateInfo);

}


VulkanPipelineLayout::~VulkanPipelineLayout()
{
    m_vulkanDevice->GetVkDevice().destroyPipelineLayout(m_vkPipelineLayout);
    m_vkPipelineLayout = nullptr;
}