#include "VulkanDescriptorPool.h"
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

VulkanDescriptorPool::VulkanDescriptorPool(
    VulkanDevice* vulkanDevice, std::vector<vk::DescriptorPoolSize> poolSizes, uint32_t maxSets)
    : m_vulkanDevice(vulkanDevice)
{
    ZoneScopedN("VulkanDescriptorPool::VulkanDescriptorPool");
    vk::DescriptorPoolCreateInfo info;
    info.setPoolSizes(poolSizes).setMaxSets(maxSets);

    m_vkDescriptorPool = m_vulkanDevice->GetVkDevice().createDescriptorPool(info);
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
    ZoneScopedN("VulkanDescriptorPool::~VulkanDescriptorPool");
    m_vulkanDevice->GetVkDevice().destroyDescriptorPool(m_vkDescriptorPool);
}

std::shared_ptr<VulkanDescriptorSets> VulkanDescriptorPool::AllocUniformDescriptorSet(
    VulkanDescriptorSetLayout* layout, const std::vector<VulkanBuffer*>& uniformBuffers,
    const std::vector<uint32_t>& binding, const std::vector<uint32_t>& range, int descriptorNum)
{
    ZoneScopedN("VulkanDescriptorPool::AllocUniformDescriptorSet");
    return std::make_shared<VulkanDescriptorSets>(
        m_vulkanDevice, m_vkDescriptorPool, layout, uniformBuffers, binding, range, descriptorNum);
}

std::shared_ptr<VulkanDescriptorSets> VulkanDescriptorPool::AllocSamplerDescriptorSet(
    VulkanDescriptorSetLayout* layout, const std::vector<VulkanImageSampler*>& imageSamplers,
    const std::vector<uint32_t>& binding, vk::ImageLayout imageLayout, int descriptorNum)
{
    ZoneScopedN("VulkanDescriptorPool::AllocSamplerDescriptorSet");
    return std::make_shared<VulkanDescriptorSets>(
        m_vulkanDevice, m_vkDescriptorPool, layout, imageSamplers, binding, imageLayout, descriptorNum);
}

std::shared_ptr<VulkanDescriptorSets>
VulkanDescriptorPool::AllocCustomToUpdatedDescriptorSet(VulkanDescriptorSetLayout* layout, int descriptorNum)
{
    ZoneScopedN("VulkanDescriptorPool::AllocCustomToUpdatedDescriptorSet");
    return std::make_shared<VulkanDescriptorSets>(m_vulkanDevice, m_vkDescriptorPool, layout, descriptorNum);
}

VulkanDelayDescriptorPool::VulkanDelayDescriptorSetIdentifier
VulkanDelayDescriptorPool::AllocDelayDescriptorSet(VulkanDescriptorSetLayout* layout)
{
    ZoneScopedN("VulkanDelayDescriptorPool::AllocDelayDescriptorSet");
    VulkanDelayDescriptorSetIdentifier id = m_delayDescriptorSets.size();
    m_delayDescriptorSets.emplace_back(std::make_unique<VulkanDelayDescriptorSet>(layout));
    return id;
}

void VulkanDelayDescriptorPool::addBufferBindingSlot(
    VulkanDelayDescriptorSetIdentifier id, vk::DescriptorType type, VulkanBuffer* buffer, uint32_t binding,
    uint64_t range, uint64_t offset)
{
    ZoneScopedN("VulkanDelayDescriptorPool::addBufferBindingSlot");
    m_delayDescriptorSets[id]->addBufferBindingSlot(type, binding, buffer, range, offset);
}

void VulkanDelayDescriptorPool::addSamplerBindingSlot(
    VulkanDelayDescriptorSetIdentifier id, vk::DescriptorType type, VulkanImageSampler* imageSampler, uint32_t binding,
    vk::ImageLayout imageLayout)
{
    ZoneScopedN("VulkanDelayDescriptorPool::addSamplerBindingSlot");
    m_delayDescriptorSets[id]->addSamplerBindingSlot(type, binding, imageSampler, imageLayout);
}

std::unique_ptr<VulkanDescriptorPool> VulkanDelayDescriptorPool::compilePool()
{
    ZoneScopedN("VulkanDelayDescriptorPool::compilePool");
    std::map<vk::DescriptorType, uint32_t> poolSizeMap;
    uint32_t maxSets = m_delayDescriptorSets.size();
    for (auto& delayDescriptorSet : m_delayDescriptorSets)
    {
        auto& slots = delayDescriptorSet->getBindingSlots();
        for (auto& [binding, slot] : slots)
        {
            poolSizeMap[slot.type]++;
        }
    }
    std::vector<vk::DescriptorPoolSize> poolSizes;
    for (auto& [type, count] : poolSizeMap)
    {
        vk::DescriptorPoolSize poolSize;
        poolSize.setType(type).setDescriptorCount(count);
        poolSizes.push_back(poolSize);
    }

    return std::make_unique<VulkanDescriptorPool>(m_device, poolSizes, maxSets);
}

std::vector<std::shared_ptr<VulkanDescriptorSets>>
VulkanDelayDescriptorPool::compileDescriptorSets(VulkanDescriptorPool* pool)
{
    std::vector<std::shared_ptr<VulkanDescriptorSets>> result;

    std::vector<vk::WriteDescriptorSet> writdescs;
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    std::vector<vk::DescriptorImageInfo> imageInfos;

    for (int i = 0; i < m_delayDescriptorSets.size(); i++)
    {
        auto& delayDescriptorSet = m_delayDescriptorSets[i];
        auto& slots = delayDescriptorSet->getBindingSlots();

        result.emplace_back(pool->AllocCustomToUpdatedDescriptorSet(delayDescriptorSet->getLayout(), 1));

        for (auto& [binding, slot] : slots)
        {
            writdescs.push_back(vk::WriteDescriptorSet()
                                    .setDescriptorType(slot.type)
                                    .setDescriptorCount(1)
                                    .setDstArrayElement(0)
                                    .setDstBinding(binding)
                                    .setDstSet(result.back()->GetVkDescriptorSet(0)));
            if (slot.type == vk::DescriptorType::eUniformBuffer || slot.type == vk::DescriptorType::eStorageBuffer)
            {
                bufferInfos.push_back(vk::DescriptorBufferInfo()
                                          .setBuffer(*slot.buffer->GetPVkBuf())
                                          .setOffset(vk::DeviceSize{slot.bufferOffset.value()})
                                          .setRange(vk::DeviceSize{slot.bufferRange.value()}));
                writdescs.back().setBufferInfo(bufferInfos.back());
            }
            else if (slot.type == vk::DescriptorType::eCombinedImageSampler)
            {
                imageInfos.push_back(vk::DescriptorImageInfo()
                                         .setImageView(*slot.imageSampler->GetPVkImageView())
                                         .setImageLayout(slot.imageLayout.value())
                                         .setSampler(*slot.imageSampler->GetPVkSampler()));
                writdescs.back().setImageInfo(imageInfos.back());
            }
        }
    }

    m_device->GetVkDevice().updateDescriptorSets((uint32_t)writdescs.size(), writdescs.data(), 0, nullptr);
    return result;
}
