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


void VulkanDescriptorSetLayout::AddBinding(uint32_t id, vk::DescriptorSetLayoutBinding binding)
{
    if (id > m_bindings.size())
    {
        m_bindings.resize(id + 1);
    }
    m_bindings[id] = binding;
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

std::shared_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayoutPresets::OnlyMVPUBO = nullptr;
std::shared_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayoutPresets::Custom5Sampler = nullptr;
void VulkanDescriptorSetLayoutPresets::Init(VulkanDevice* device)
{
    OnlyMVPUBO = std::make_shared<VulkanMVPUBODescriptorSetLayout>(device);
    OnlyMVPUBO->Finish();
    Custom5Sampler = std::make_shared<VulkanCustom5SamplerDescriptorSetLayout>(device);
    Custom5Sampler->Finish();
}
void VulkanDescriptorSetLayoutPresets::UnInit()
{
    OnlyMVPUBO.reset();
    Custom5Sampler.reset();
}


const vk::DescriptorSetLayoutBinding& VulkanMVPUBODescriptorSetLayout::GetVkBinding()
{
    static auto defaultBinding = vk::DescriptorSetLayoutBinding()
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setDescriptorCount(1)
            .setBinding(DESCRIPTOR_MVPUBO_BINDING_ID);
    return defaultBinding;
}

void VulkanMVPUBODescriptorSetLayout::Finish()
{
    if (m_vkDescriptorSetLayout)
    {
        return;
    }
    static auto layoutInfo = vk::DescriptorSetLayoutCreateInfo()
                .setBindings(GetVkBinding());
    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}


const std::vector<vk::DescriptorSetLayoutBinding>& VulkanCustom5SamplerDescriptorSetLayout::GetVkBinding()
{
    static std::vector<vk::DescriptorSetLayoutBinding> bindings;
    if (bindings.empty())
    {
        for (int i = DESCRIPTOR_SAMPLER1_BINDING_ID; i <= DESCRIPTOR_SAMPLER5_BINDING_ID; i++)
        {
            auto samplerBinding = vk::DescriptorSetLayoutBinding()
                        .setBinding(i)
                        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            bindings.emplace_back(samplerBinding);
        }
    }

    return bindings;
}
void VulkanCustom5SamplerDescriptorSetLayout::Finish()
{
    if (m_vkDescriptorSetLayout)
    {
        return;
    }
    static auto layoutInfo = vk::DescriptorSetLayoutCreateInfo()
                .setBindings(GetVkBinding());
    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}