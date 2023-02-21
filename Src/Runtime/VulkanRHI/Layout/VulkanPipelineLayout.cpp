#include "VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <algorithm>
#include <string.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING



VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descLayouts)
    : m_vulkanDevice(device)
    , m_vulkanDescSetLayouts(descLayouts)
{
    std::vector<vk::DescriptorSetLayout> vkLayouts;
    m_DescriptorSetLayoutNames.resize(descLayouts.size());
    for (int i = 0; i < descLayouts.size(); i++)
    {
        auto layout = descLayouts[i];
        vk::DescriptorSetLayout vkLayout;
        if (layout)
        {
            m_DescriptorSetLayoutNames[i] = layout->GetName();
            vkLayout = layout->GetVkDescriptorSetLayout();
        }
        vkLayouts.emplace_back(vkLayout);
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts(vkLayouts)
                            ; //.setPushConstantRanges(m_vkPushConstRanges);

    m_vkPipelineLayout = m_vulkanDevice->GetVkDevice().createPipelineLayout(pipelineLayoutCreateInfo);

}


VulkanPipelineLayout::~VulkanPipelineLayout()
{
    m_vulkanDevice->GetVkDevice().destroyPipelineLayout(m_vkPipelineLayout);
    m_vkPipelineLayout = nullptr;
}

int VulkanPipelineLayout::GetDescriptorSetId(const char* descLayoutName)
{
    for (int id = 0; id < m_DescriptorSetLayoutNames.size(); id++)
    {
        if (strcmp(m_DescriptorSetLayoutNames[id], descLayoutName) == 0)
        {
            return id;
        }
    }
    return -1;
}

int VulkanPipelineLayout::GetDescriptorSetId(VulkanDescriptorSetLayout* layout)
{
    assert(layout);
    return GetDescriptorSetId(layout->GetName());
}

int VulkanPipelineLayout::GetDescriptorSetId(VulkanDescriptorSets* desc)
{
    assert(desc);
    return GetDescriptorSetId(desc->GetPDescriptorSetLayout());
}