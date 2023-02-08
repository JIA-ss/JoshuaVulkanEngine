#pragma once
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanImageSampler;
class VulkanDescriptorSets
{
public:
private:
    vk::DescriptorPool m_vkDescPool;
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanDescriptorSetLayout* m_vulkanDescLayout = nullptr;

    std::vector<vk::DescriptorSet> m_vkDescSets;
    std::vector<std::unique_ptr<VulkanBuffer>> m_vulkanUniformWriteBuffers;
    std::vector<std::unique_ptr<VulkanImageSampler>> m_vulkanImages;
public:
    explicit VulkanDescriptorSets(
        VulkanDevice* device,
        VulkanDescriptorSetLayout* layout,
        std::vector<std::unique_ptr<VulkanBuffer>>&& uniformbuffers,
        std::vector<std::unique_ptr<VulkanImageSampler>>&& vulkanImages);
    ~VulkanDescriptorSets();
    inline std::vector<vk::DescriptorSet>& GetVkDescriptorSets() { return m_vkDescSets; }
    inline vk::DescriptorSet& GetVkDescriptorSet(int idx) { return m_vkDescSets[idx]; }
    inline VulkanBuffer* GetPWriteUniformBuffer(int idx) { return m_vulkanUniformWriteBuffers[idx].get(); }
    inline std::vector<std::unique_ptr<VulkanBuffer>>& GetUniformBuffers() { return m_vulkanUniformWriteBuffers; }
};
RHI_NAMESPACE_END