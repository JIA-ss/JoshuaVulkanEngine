#include "VulkanDescriptorPool.h"
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"

RHI_NAMESPACE_USING

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* vulkanDevice, std::vector<vk::DescriptorPoolSize> poolSizes, uint32_t maxSets)
    : m_vulkanDevice(vulkanDevice)
{
    vk::DescriptorPoolCreateInfo info;
    info.setPoolSizes(poolSizes)
        .setMaxSets(maxSets);

    m_vkDescriptorPool = m_vulkanDevice->GetVkDevice().createDescriptorPool(info);
}


VulkanDescriptorPool::~VulkanDescriptorPool()
{
    m_vulkanDevice->GetVkDevice().destroyDescriptorPool(m_vkDescriptorPool);
}

std::shared_ptr<VulkanDescriptorSets> VulkanDescriptorPool::AllocUniformDescriptorSet(
    VulkanDescriptorSetLayout* layout,
    const std::vector<VulkanBuffer*>& uniformBuffers,
    const std::vector<uint32_t>& binding)
{
    return std::make_shared<VulkanDescriptorSets>(m_vulkanDevice, m_vkDescriptorPool, layout, uniformBuffers, binding);
}

std::shared_ptr<VulkanDescriptorSets> VulkanDescriptorPool::AllocSamplerDescriptorSet(
    VulkanDescriptorSetLayout* layout,
    const std::vector<VulkanImageSampler*>& imageSamplers,
    const std::vector<uint32_t>& binding)
{
    return std::make_shared<VulkanDescriptorSets>(m_vulkanDevice, m_vkDescriptorPool, layout, imageSamplers, binding);
}