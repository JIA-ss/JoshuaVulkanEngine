#include "VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSets::VulkanDescriptorSets(VulkanDevice* device, VulkanDescriptorSetLayout* layout, vk::DescriptorPoolSize size)
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_size(size)
{
    vk::DescriptorPoolCreateInfo info;
    info.setPoolSizes(m_size)
        .setPoolSizeCount(1)
        .setMaxSets(m_size.descriptorCount);

    m_vkDescPool = m_vulkanDevice->GetVkDevice().createDescriptorPool(info);
    std::vector<vk::DescriptorSetLayout> layouts(m_size.descriptorCount, layout->GetVkDescriptorSetLayout());
    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(m_vkDescPool)
                .setDescriptorSetCount(m_size.descriptorCount)
                .setSetLayouts(layouts);
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    m_vulkanUniformWriteBuffers.resize(m_vkDescSets.size());

    for (int i = 0; i < m_vkDescSets.size(); i++)
    {
        m_vulkanUniformWriteBuffers[i].reset(
            new VulkanBuffer(
                    device, sizeof(UniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                )
        );

        auto descBufInfo = vk::DescriptorBufferInfo()
                    .setBuffer(*m_vulkanUniformWriteBuffers[i]->GetPVkBuf())
                    .setOffset(0)
                    .setRange(sizeof(UniformBufferObject));
        auto descWrite = vk::WriteDescriptorSet()
                    .setDstSet(m_vkDescSets[i])
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                    .setDescriptorCount(1)
                    .setBufferInfo(descBufInfo)
                    ;

        m_vulkanDevice->GetVkDevice().updateDescriptorSets(1, &descWrite, 0, nullptr);
    }
}


VulkanDescriptorSets::~VulkanDescriptorSets()
{
    m_vulkanDevice->GetVkDevice().destroyDescriptorPool(m_vkDescPool);
}