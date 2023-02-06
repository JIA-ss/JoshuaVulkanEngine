#include "VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device)
    : m_vulkanDevice(device)
{


}


void VulkanDescriptorSetLayout::AddBindings(std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    for (vk::DescriptorSetLayoutBinding& binding : bindings)
    {
        binding.setBinding(m_bindings.size());
        m_bindings.emplace_back(binding);
    }
}

void VulkanDescriptorSetLayout::Finish()
{
    if (m_vkDescriptorSetLayout)
    {
        return;
    }
    auto layoutInfo = vk::DescriptorSetLayoutCreateInfo()
                .setBindings(m_bindings);

    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}

vk::DescriptorSetLayout& VulkanDescriptorSetLayout::GetVkDescriptorSetLayout()
{
    Finish();
    return m_vkDescriptorSetLayout;
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
    m_vulkanDevice->GetVkDevice().destroyDescriptorSetLayout(m_vkDescriptorSetLayout);
    m_vkDescriptorSetLayout= nullptr;
}