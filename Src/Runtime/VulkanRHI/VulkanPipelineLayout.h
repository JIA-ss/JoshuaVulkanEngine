#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"

RHI_NAMESPACE_BEGIN

class VulkanDescriptorSetLayout;
class VulkanDevice;
class VulkanPipelineLayout
{
public:
private:
    VulkanDescriptorSetLayout* m_vulkanDescSetLayout = nullptr;
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::PipelineLayout m_vkPipelineLayout;
    std::vector<vk::PushConstantRange> m_vkPushConstRanges;
public:
    explicit VulkanPipelineLayout(VulkanDevice* device, VulkanDescriptorSetLayout* descLayout);
    ~VulkanPipelineLayout();
};
RHI_NAMESPACE_END