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



VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descLayouts, const std::map<int, vk::PushConstantRange>& pushConstant)
    : m_vulkanDevice(device)
    , m_vulkanDescSetLayouts(descLayouts)
    , m_vkPushConstRanges(pushConstant)
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

    std::vector<vk::PushConstantRange> pushConstantRanges;
    bool constantValid = checkPushContantRangeValid(pushConstantRanges);
    assert(constantValid);

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts(vkLayouts)
                            .setPushConstantRanges(pushConstantRanges)
                            ;

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

void VulkanPipelineLayout::PushConstant(vk::CommandBuffer cmd, uint32_t offset, uint32_t size, void* data, vk::ShaderStageFlags stage)
{
    auto it = m_vkPushConstRanges.find(offset);
    assert(it != m_vkPushConstRanges.end());
    assert(it->second.size >= size);
    assert(data != nullptr);
    cmd.pushConstants(m_vkPipelineLayout, stage, offset, size, data);
}

bool VulkanPipelineLayout::checkPushContantRangeValid(std::vector<vk::PushConstantRange>& validRanges)
{
    validRanges.clear();
    validRanges.reserve(m_vkPushConstRanges.size());

    for (auto& [offset, range] : m_vkPushConstRanges)
    {
        validRanges.emplace_back(range);
    }

    if (validRanges.empty())
    {
        return true;
    }

    for (int i = 0; i < validRanges.size() - 1; i++)
    {
        if (validRanges[i].offset + validRanges[i].size > validRanges[i + 1].offset)
        {
            return false;
        }
    }

    return true;
}