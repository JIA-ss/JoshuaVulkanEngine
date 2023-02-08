#include "VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSets::VulkanDescriptorSets(
    VulkanDevice* device,
    VulkanDescriptorSetLayout* layout,
    std::vector<std::unique_ptr<VulkanBuffer>>&& uniformbuffers,
    std::vector<std::unique_ptr<VulkanImage>>&& vulkanImages)
    : m_vulkanDevice(device)
    , m_vulkanDescLayout(layout)
    , m_vulkanUniformWriteBuffers(std::move(uniformbuffers))
    , m_vulkanImages(std::move(vulkanImages))
{

    std::vector<vk::DescriptorPoolSize> sizes;
    if (!m_vulkanUniformWriteBuffers.empty())
    {
        sizes.emplace_back(vk::DescriptorPoolSize
        {
            vk::DescriptorType::eUniformBuffer,
            MAX_FRAMES_IN_FLIGHT
        });
    }
    if (!m_vulkanImages.empty())
    {
        sizes.emplace_back(vk::DescriptorPoolSize
        {
            vk::DescriptorType::eCombinedImageSampler,
            MAX_FRAMES_IN_FLIGHT
        });
    }

    vk::DescriptorPoolCreateInfo info;
    info.setPoolSizes(sizes)
        .setMaxSets(MAX_FRAMES_IN_FLIGHT);

    m_vkDescPool = m_vulkanDevice->GetVkDevice().createDescriptorPool(info);
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout->GetVkDescriptorSetLayout());
    auto allocInfo = vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(m_vkDescPool)
                .setDescriptorSetCount(MAX_FRAMES_IN_FLIGHT)
                .setSetLayouts(layouts);
    m_vkDescSets = m_vulkanDevice->GetVkDevice().allocateDescriptorSets(allocInfo);
    assert(m_vulkanUniformWriteBuffers.size() == m_vkDescSets.size());

    for (int i = 0; i < m_vkDescSets.size(); i++)
    {
        std::vector<vk::WriteDescriptorSet> writeDescs;
        uint32_t dstBinding = 0;
        if (!m_vulkanUniformWriteBuffers.empty())
        {
            auto descBufInfo = vk::DescriptorBufferInfo()
                        .setBuffer(*m_vulkanUniformWriteBuffers[i]->GetPVkBuf())
                        .setOffset(0)
                        .setRange(sizeof(UniformBufferObject))
                        ;
            auto writeDesc = vk::WriteDescriptorSet()
                        .setDstSet(m_vkDescSets[i])
                        .setDstBinding(dstBinding++)
                        .setDstArrayElement(0)
                        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                        .setDescriptorCount(1)
                        .setBufferInfo(descBufInfo)
                        ;
            writeDescs.emplace_back(writeDesc);
        }

        if (!m_vulkanImages.empty())
        {

            auto descImgInfo = vk::DescriptorImageInfo()
                        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                        .setImageView(*m_vulkanImages[i]->GetPVkImageView())
                        .setSampler(*m_vulkanImages[i]->GetPVkSampler())
                        ;
            auto writeDesc = vk::WriteDescriptorSet()
                        .setDstSet(m_vkDescSets[i])
                        .setDstBinding(dstBinding++)
                        .setDstArrayElement(0)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setDescriptorCount(1)
                        .setImageInfo(descImgInfo)
                        ;
            writeDescs.emplace_back(writeDesc);
        }

        m_vulkanDevice->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
    }
}


VulkanDescriptorSets::~VulkanDescriptorSets()
{
    m_vulkanDevice->GetVkDevice().destroyDescriptorPool(m_vkDescPool);
}