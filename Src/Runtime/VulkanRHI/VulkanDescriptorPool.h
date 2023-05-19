#pragma once
#include <stdint.h>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"

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
    explicit VulkanDescriptorPool(
        VulkanDevice* vulkanDevice, std::vector<vk::DescriptorPoolSize> poolSizes, uint32_t maxSets);
    ~VulkanDescriptorPool();

    std::shared_ptr<VulkanDescriptorSets> AllocUniformDescriptorSet(
        VulkanDescriptorSetLayout* layout, const std::vector<VulkanBuffer*>& uniformBuffers,
        const std::vector<uint32_t>& binding, const std::vector<uint32_t>& range, int descriptorNum = 1);

    std::shared_ptr<VulkanDescriptorSets> AllocSamplerDescriptorSet(
        VulkanDescriptorSetLayout* layout, const std::vector<VulkanImageSampler*>& imageSamplers,
        const std::vector<uint32_t>& binding, vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        int descriptorNum = 1);

    std::shared_ptr<VulkanDescriptorSets>
    AllocCustomToUpdatedDescriptorSet(VulkanDescriptorSetLayout* layout, int descriptorNum = 1);
};

class VulkanDelayDescriptorSet;
class VulkanDelayDescriptorPool
{
public:
    using VulkanDelayDescriptorSetIdentifier = uint32_t;

public:
    explicit VulkanDelayDescriptorPool(VulkanDevice* vulkanDevice) : m_device(vulkanDevice) {}
    ~VulkanDelayDescriptorPool();

    VulkanDelayDescriptorSetIdentifier AllocDelayDescriptorSet(VulkanDescriptorSetLayout* layout);
    void addBufferBindingSlot(
        VulkanDelayDescriptorSetIdentifier id, vk::DescriptorType type, VulkanBuffer* buffer, uint32_t binding,
        uint64_t range, uint64_t offset);
    void addSamplerBindingSlot(
        VulkanDelayDescriptorSetIdentifier id, vk::DescriptorType type, VulkanImageSampler* image, uint32_t binding,
        vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal);

    std::unique_ptr<VulkanDescriptorPool> compilePool();
    std::vector<std::shared_ptr<VulkanDescriptorSets>> compileDescriptorSets(VulkanDescriptorPool* pool);

private:
    VulkanDevice* m_device;
    std::vector<std::unique_ptr<VulkanDelayDescriptorSet>> m_delayDescriptorSets;
};
RHI_NAMESPACE_END