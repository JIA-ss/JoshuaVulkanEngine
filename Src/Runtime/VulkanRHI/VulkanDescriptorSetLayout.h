#pragma once
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/VulkanRHI.h"


RHI_NAMESPACE_BEGIN

class VulkanShaderSet;
class VulkanDevice;
class VulkanDescriptorSetLayout
{
public:
private:
    VulkanShaderSet* m_vulkanShaderSet = nullptr;
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::DescriptorSetLayout m_vkDescriptorSetLayout;
public:
    explicit VulkanDescriptorSetLayout(VulkanDevice* device, VulkanShaderSet* shaderset);
    ~VulkanDescriptorSetLayout();

    inline vk::DescriptorSetLayout& GetVkDescriptorSetLayout() { return m_vkDescriptorSetLayout; }
};
RHI_NAMESPACE_END