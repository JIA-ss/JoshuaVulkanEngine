#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_handles.hpp"

RHI_NAMESPACE_BEGIN

class VulkanDescriptorSets;
class VulkanDescriptorSetLayout;
class VulkanDevice;
class VulkanPipelineLayout
{
public:
private:
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_vulkanDescSetLayouts;
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::PipelineLayout m_vkPipelineLayout;
    std::vector<vk::PushConstantRange> m_vkPushConstRanges;
    std::vector<const char*> m_DescriptorSetLayoutNames;
public:
    explicit VulkanPipelineLayout(VulkanDevice* device, std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descLayouts);
    ~VulkanPipelineLayout();

    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> GetVulkanDescriptorSetLayouts() { return m_vulkanDescSetLayouts; }
    inline vk::PipelineLayout& GetVkPieplineLayout() { return m_vkPipelineLayout; }

    int GetDescriptorSetId(const char* descLayoutName);
    int GetDescriptorSetId(VulkanDescriptorSetLayout* layout);
    int GetDescriptorSetId(VulkanDescriptorSets* desc);
};
RHI_NAMESPACE_END