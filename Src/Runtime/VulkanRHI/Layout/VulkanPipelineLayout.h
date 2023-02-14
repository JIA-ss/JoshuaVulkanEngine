#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_handles.hpp"

RHI_NAMESPACE_BEGIN

class VulkanDescriptorSetLayout;
class VulkanDevice;
class VulkanPipelineLayout
{
public:
private:
    std::weak_ptr<VulkanDescriptorSetLayout> m_vulkanDescSetLayout;
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::PipelineLayout m_vkPipelineLayout;
    std::vector<vk::PushConstantRange> m_vkPushConstRanges;
public:
    explicit VulkanPipelineLayout(VulkanDevice* device, std::shared_ptr<VulkanDescriptorSetLayout> descLayout);
    ~VulkanPipelineLayout();

    inline vk::PipelineLayout& GetVkPieplineLayout() { return m_vkPipelineLayout; }
};
RHI_NAMESPACE_END