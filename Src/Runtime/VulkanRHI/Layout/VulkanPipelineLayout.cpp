#include "VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <algorithm>
#include <string.h>
#include <string>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

std::optional<VulkanPipelineLayout::BindingInfo>
VulkanPipelineLayout::BindingInfo::Merge(const VulkanPipelineLayout::BindingInfo& other)
{
    auto maxSetBindingSize = std::max(setBindings.size(), other.setBindings.size());
    VulkanPipelineLayout::BindingInfo result;
    result.setBindings.resize(maxSetBindingSize);

    auto minSetBindingSize = std::min(setBindings.size(), other.setBindings.size());

    auto createBindingSlotVector = [](const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    {
        std::vector<std::optional<vk::DescriptorSetLayoutBinding>> mappingBindings;
        for (int i = 0; i < bindings.size(); i++)
        {
            auto bindingId = bindings[i].binding;
            if (bindingId >= mappingBindings.size())
            {
                mappingBindings.resize(bindingId + 1);
            }
            mappingBindings[bindingId] = bindings[i];
        }
        return mappingBindings;
    };

    for (int setId = 0; setId < minSetBindingSize; setId++)
    {
        auto bindings = createBindingSlotVector(this->setBindings[setId]);
        auto otherBindings = createBindingSlotVector(other.setBindings[setId]);

        auto minBindingSize = std::min(bindings.size(), otherBindings.size());

        for (int bindingId = 0; bindingId < minSetBindingSize; bindingId++)
        {
            auto& binding = bindings[bindingId];
            auto& otherBinding = otherBindings[bindingId];

            if (!binding.has_value())
            {
                if (otherBinding.has_value())
                {
                    result.setBindings[setId].push_back(otherBinding.value());
                }
                continue;
            }

            if (!otherBinding.has_value())
            {
                result.setBindings[setId].push_back(binding.value());
                continue;
            }

            if (binding->binding != otherBinding->binding)
            {
                return std::nullopt;
            }

            if (binding->descriptorType != otherBinding->descriptorType)
            {
                return std::nullopt;
            }

            vk::DescriptorSetLayoutBinding vkBinding = binding.value();
            vkBinding.setDescriptorCount(std::max(binding->descriptorCount, otherBinding->descriptorCount))
                .setStageFlags(binding->stageFlags | otherBinding->stageFlags);
            result.setBindings[setId].push_back(vkBinding);
        }
    }

    result.pushConstant.resize(pushConstant.size() + other.pushConstant.size());
    std::copy(pushConstant.begin(), pushConstant.end(), result.pushConstant.begin());
    std::copy(other.pushConstant.begin(), other.pushConstant.end(), result.pushConstant.begin() + pushConstant.size());
    return result;
}

VulkanPipelineLayout::VulkanPipelineLayout(
    VulkanDevice* device, std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descLayouts,
    const std::map<int, vk::PushConstantRange>& pushConstant)
    : m_vulkanDevice(device), m_vulkanDescSetLayouts(descLayouts), m_vkPushConstRanges(pushConstant)
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
    pipelineLayoutCreateInfo.setSetLayouts(vkLayouts).setPushConstantRanges(pushConstantRanges);

    m_vkPipelineLayout = m_vulkanDevice->GetVkDevice().createPipelineLayout(pipelineLayoutCreateInfo);
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, const BindingInfo& bindingInfo)
    : m_vulkanDevice(device)
{
    std::string layoutName = "layout ";
    int setId = 0;
    std::vector<vk::DescriptorSetLayout> vkLayouts;
    for (auto& setBinding : bindingInfo.setBindings)
    {
        std::shared_ptr<VulkanDescriptorSetLayout> layout =
            std::make_shared<VulkanDescriptorSetLayout>(m_vulkanDevice, setBinding);

        vkLayouts.emplace_back(layout->GetVkDescriptorSetLayout());
        m_vulkanDescSetLayouts.emplace_back(layout);
        m_DescriptorSetLayoutNames.push_back((layoutName + std::to_string(setId++)).c_str());
    }

    // todo: recreate push constant range

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setSetLayouts(vkLayouts).setPushConstantRanges(bindingInfo.pushConstant);

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

void VulkanPipelineLayout::PushConstant(
    vk::CommandBuffer cmd, uint32_t offset, uint32_t size, void* data, vk::ShaderStageFlags stage)
{
    auto it = m_vkPushConstRanges.find(offset);
    assert(it != m_vkPushConstRanges.end());
    assert(it->second.size >= size);
    assert(data != nullptr);
    cmd.pushConstants(m_vkPipelineLayout, stage, offset, size, data);
}

VulkanPipelineLayout::BindingInfo VulkanPipelineLayout::GetBindingInfo()
{
    BindingInfo bindingInfo;
    checkPushContantRangeValid(bindingInfo.pushConstant);
    bindingInfo.setBindings.resize(m_vulkanDescSetLayouts.size());
    for (int setId = 0; setId < m_vulkanDescSetLayouts.size(); setId++)
    {
        auto& setLayout = m_vulkanDescSetLayouts[setId];
        if (setLayout)
        {
            bindingInfo.setBindings[setId] = setLayout->GetVkBindings();
        }
    }
    return bindingInfo;
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