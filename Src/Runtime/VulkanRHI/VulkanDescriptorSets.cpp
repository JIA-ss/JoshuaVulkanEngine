#include "VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSets::VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        const std::vector<VulkanBuffer*>& uniformBuffers,
        const std::vector<uint32_t>& binding
    )
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_binding(binding)
    , m_vkDescPool(descPool)
{
    assert(binding.size() == uniformBuffers.size());

    std::vector<vk::DescriptorSetLayout> layouts(uniformBuffers.size(), layout->GetVkDescriptorSetLayout());
    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descPool)
                .setDescriptorSetCount(uniformBuffers.size())
                .setSetLayouts(layouts);
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(uniformBuffers.size() == m_vkDescSets.size());
    std::vector<vk::WriteDescriptorSet> writeDescs;
    std::vector<vk::DescriptorBufferInfo> bufferInfo(m_vkDescSets.size());
    for (int i = 0; i < m_vkDescSets.size(); i++)
    {
        bufferInfo[i] = vk::DescriptorBufferInfo()
                    .setBuffer(*uniformBuffers[i]->GetPVkBuf())
                    .setOffset(0)
                    .setRange(sizeof(UniformBufferObject))
                    ;
        auto writeDesc = vk::WriteDescriptorSet()
                    .setDstSet(m_vkDescSets[i])
                    .setDstBinding(binding[i])
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setBufferInfo(bufferInfo[i])
                    ;
        writeDescs.emplace_back(writeDesc);
    }
    m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
}


VulkanDescriptorSets::VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        std::vector<VulkanImageSampler*> imageSamplers,
        const std::vector<uint32_t>& binding
    )
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_binding(binding)
    , m_vkDescPool(descPool)
{

    assert(binding.size() == imageSamplers.size());


    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descPool)
                .setDescriptorSetCount(1)
                .setSetLayouts(layout->GetVkDescriptorSetLayout());
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(m_vkDescSets.size() == 1);

    std::vector<vk::WriteDescriptorSet> writeDescs(imageSamplers.size());
    std::vector<vk::DescriptorImageInfo> imageInfo(imageSamplers.size());
    for (int i = 0; i < imageSamplers.size(); i++)
    {
        imageInfo[i] = vk::DescriptorImageInfo()
                    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setImageView(*imageSamplers[i]->GetPVkImageView())
                    .setSampler(*imageSamplers[i]->GetPVkSampler())
                    ;
        writeDescs[i] = vk::WriteDescriptorSet()
                    .setDstSet(m_vkDescSets[0])
                    .setDstBinding(binding[i])
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDescriptorCount(1)
                    .setImageInfo(imageInfo[i])
                    ;
    }
    m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
}

VulkanDescriptorSets::~VulkanDescriptorSets()
{
    m_vulkanDevice->GetVkDevice().freeDescriptorSets(m_vkDescPool, m_vkDescSets);
}

void VulkanDescriptorSets::FillToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout, int selfSetIndex)
{
    vk::DescriptorSet tobinding = m_vkDescSets[selfSetIndex];
    int setId = pipelineLayout->GetDescriptorSetId(this);
    if (setId != -1)
    {
        if (descList.size() <= setId)
        {
            descList.resize(setId + 1);
        }
        descList[setId] = tobinding;
    }
}

void VulkanDescriptorSets::BindGraphicPipelinePoint(vk::CommandBuffer cmd, vk::PipelineLayout layout, const std::vector<int>& setIdx, int firstIdx)
{
    std::vector<vk::DescriptorSet> toBindSets;
    if (setIdx.empty())
    {
        toBindSets = m_vkDescSets;
    }
    else
    {
        for (auto& idx : setIdx)
        {
            toBindSets.push_back(m_vkDescSets[idx]);
        }
    }
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, firstIdx, toBindSets, {});
}

std::vector<vk::DescriptorSet> VulkanDescriptorSets::GetVkDescriptorSets(const std::vector<int>& setIdx)
{
    std::vector<vk::DescriptorSet> toBindSets;
    if (setIdx.empty())
    {
        toBindSets = m_vkDescSets;
    }
    else
    {
        for (auto& idx : setIdx)
        {
            toBindSets.push_back(m_vkDescSets[idx]);
        }
    }
    return toBindSets;
}