#include "VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device, VulkanShaderSet* shaderset)
    : m_vulkanShaderSet(shaderset)
    , m_vulkanDevice(device)
{
    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindings(shaderset->GetDescriptorSetLayoutBindings());

    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}


VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    m_vulkanDevice->GetVkDevice().destroyDescriptorSetLayout(m_vkDescriptorSetLayout);
    m_vkDescriptorSetLayout= nullptr;
}