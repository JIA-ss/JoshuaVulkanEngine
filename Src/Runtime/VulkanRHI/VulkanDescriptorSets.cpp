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
        const std::vector<uint32_t>& binding,
        const std::vector<uint32_t>& range,
        int descriptorNum
    )
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_pVulkanUniformBuffers(uniformBuffers)
    , m_binding(binding)
    , m_vkDescPool(descPool)
{
    ZoneScopedN("VulkanDescriptorSets::VulkanDescriptorSets");
    assert(binding.size() == uniformBuffers.size());
    assert(uniformBuffers.size() == range.size());

    std::vector<vk::DescriptorSetLayout> layouts(descriptorNum, layout->GetVkDescriptorSetLayout());
    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descPool)
                .setDescriptorSetCount(descriptorNum)
                .setSetLayouts(layouts);
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(descriptorNum == m_vkDescSets.size());

    std::vector<vk::WriteDescriptorSet> writeDescs;
    std::vector<vk::DescriptorBufferInfo> bufferInfo(uniformBuffers.size());
    for (vk::DescriptorSet& vkSet : m_vkDescSets)
    {
        for (int i = 0; i < uniformBuffers.size(); i++)
        {
            if (uniformBuffers[i] == nullptr)
            {
                continue;
            }
            bufferInfo[i] = vk::DescriptorBufferInfo()
                        .setBuffer(*uniformBuffers[i]->GetPVkBuf())
                        .setOffset(0)
                        .setRange(range[i])
                        ;
            auto writeDesc = vk::WriteDescriptorSet()
                        .setDstSet(vkSet)
                        .setDstBinding(binding[i])
                        .setDstArrayElement(0)
                        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                        .setDescriptorCount(1)
                        .setBufferInfo(bufferInfo[i])
                        ;
            writeDescs.emplace_back(writeDesc);
        }
    }
    m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
}


VulkanDescriptorSets::VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        std::vector<VulkanImageSampler*> imageSamplers,
        const std::vector<uint32_t>& binding,
        vk::ImageLayout imageLayout,
        int descriptorNum
    )
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_binding(binding)
    , m_pVulkanImageSamplers(imageSamplers)
    , m_vkDescPool(descPool)
{
    ZoneScopedN("VulkanDescriptorSets::VulkanDescriptorSets");
    assert(binding.size() == imageSamplers.size());


    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descPool)
                .setDescriptorSetCount(1)
                .setSetLayouts(layout->GetVkDescriptorSetLayout());
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(m_vkDescSets.size() == 1);


    std::vector<vk::DescriptorImageInfo> imageInfo;
    for (int i = 0; i < imageSamplers.size(); i++)
    {
        if (!imageSamplers[i])
        {
            continue;
        }
        imageInfo.push_back(vk::DescriptorImageInfo()
                    .setImageLayout(imageLayout)
                    .setImageView(*imageSamplers[i]->GetPVkImageView())
                    .setSampler(*imageSamplers[i]->GetPVkSampler())
        );
    }
    std::vector<vk::WriteDescriptorSet> writeDescs(imageInfo.size());
    for (int i = 0; i < imageInfo.size(); i++)
    {
        writeDescs[i]
                    .setDstSet(m_vkDescSets[0])
                    .setDstBinding(binding[i])
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDescriptorCount(1)
                    .setImageInfo(imageInfo[i]);
    }
    m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
}

VulkanDescriptorSets::VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        int descriptorNum
    )
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_vkDescPool(descPool)
{
    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descPool)
                .setDescriptorSetCount(descriptorNum)
                .setSetLayouts(layout->GetVkDescriptorSetLayout());
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(m_vkDescSets.size() == descriptorNum);
}

VulkanDescriptorSets::~VulkanDescriptorSets()
{
    /*
        if descriptorPool is created by VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag
        descriptorSets created by the pool need be free manually
    */
    // m_vulkanDevice->GetVkDevice().freeDescriptorSets(m_vkDescPool, m_vkDescSets);
}

void VulkanDescriptorSets::UpdateDescriptorSets(std::vector<vk::WriteDescriptorSet>& writeDescs)
{
    assert(m_vkDescSets.size() == 1);
    for (int i = 0; i < writeDescs.size(); i++)
    {
        writeDescs[i].setDstSet(m_vkDescSets[0]);
    }
    m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
}


void VulkanDescriptorSets::FillToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout, int selfSetIndex)
{
    ZoneScopedN("VulkanDescriptorSets::FillToBindedDescriptorSetsVector");
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