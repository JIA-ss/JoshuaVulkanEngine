#pragma once
#include <stdint.h>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanDescriptorSets;
class VulkanDescriptorPool
{
public:
private:
    VulkanDevice* m_vulkanDevice;
    vk::DescriptorPool m_vkDescriptorPool;
public:
    explicit VulkanDescriptorPool(VulkanDevice* vulkanDevice, std::vector<vk::DescriptorPoolSize> poolSizes, uint32_t maxSets);
    ~VulkanDescriptorPool();

    std::shared_ptr<VulkanDescriptorSets> AllocUniformDescriptorSet(
                VulkanDescriptorSetLayout* layout,
                const std::vector<VulkanBuffer*>& uniformBuffers,
                const std::vector<uint32_t>& binding,
                const std::vector<uint32_t>& range,
                int descriptorNum = 1
            );

    std::shared_ptr<VulkanDescriptorSets> AllocSamplerDescriptorSet(
                VulkanDescriptorSetLayout* layout,
                const std::vector<VulkanImageSampler*>& imageSamplers,
                const std::vector<uint32_t>& binding,
                vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                int descriptorNum = 1
            );
};
RHI_NAMESPACE_END