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
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
public:
    explicit VulkanDescriptorSetLayout(VulkanDevice* device);
    ~VulkanDescriptorSetLayout();
    void AddBindings(std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    void Finish();
    vk::DescriptorSetLayout& GetVkDescriptorSetLayout();
};
RHI_NAMESPACE_END