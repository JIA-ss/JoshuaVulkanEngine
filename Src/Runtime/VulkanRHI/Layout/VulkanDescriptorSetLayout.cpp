#include "VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice* device) : m_vulkanDevice(device) {}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    VulkanDevice* device, const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
    : m_vulkanDevice(device), m_bindings(bindings)
{
    VulkanDescriptorSetLayout::Finish();
}

void VulkanDescriptorSetLayout::AddBinding(uint32_t id, vk::DescriptorSetLayoutBinding binding)
{
    if (id >= m_bindings.size())
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
    auto layoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);

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
    m_vkDescriptorSetLayout = nullptr;
}

std::shared_ptr<VulkanDescriptorSetLayout>
VulkanDescriptorSetLayoutPresets::CreateCustomUBO(VulkanDevice* device, vk::ShaderStageFlags stage)
{
    std::vector<vk::DescriptorSetLayoutBinding> binding = VulkanUBODescriptorSetLayout::GetVkBinding();
    assert(binding.size() == 3);
    binding.push_back(vk::DescriptorSetLayoutBinding()
                          .setBinding(VulkanDescriptorSetLayout::DESCRIPTOR_CUSTOMUBO_BINDING_ID)
                          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                          .setDescriptorCount(1)
                          .setStageFlags(stage));

    return std::make_shared<VulkanDescriptorSetLayout>(device, binding);
}

std::shared_ptr<VulkanDescriptorSetLayout>
VulkanDescriptorSetLayoutPresets::CreateSBO(VulkanDevice* device, vk::ShaderStageFlags stage, uint32_t num)
{
    static std::vector<vk::DescriptorSetLayoutBinding> binding;
    if (binding.empty())
    {
        binding.resize(num);
        for (int i = 0; i < num; i++)
        {
            binding[i]
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setStageFlags(stage)
                .setDescriptorCount(1)
                .setBinding(i);
        }
    }

    return std::make_shared<VulkanDescriptorSetLayout>(device, binding);
}

void VulkanDescriptorSetLayoutPresets::Init(VulkanDevice* device)
{
    UBO = std::make_shared<VulkanUBODescriptorSetLayout>(device);
    UBO->Finish();
    CUSTOM5SAMPLER = std::make_shared<VulkanCustom5SamplerDescriptorSetLayout>(device);
    CUSTOM5SAMPLER->Finish();
    SHADOWMAP = std::make_shared<VulkanShadowMapDescriptorSetLayout>(device);
    SHADOWMAP->Finish();
}
void VulkanDescriptorSetLayoutPresets::UnInit()
{
    UBO.reset();
    CUSTOM5SAMPLER.reset();
    SHADOWMAP.reset();
}

const std::vector<vk::DescriptorSetLayoutBinding>& VulkanUBODescriptorSetLayout::GetVkBinding()
{
    static std::vector<vk::DescriptorSetLayoutBinding> defaultBinding;
    if (defaultBinding.empty())
    {
        defaultBinding.resize(3);
        defaultBinding[0]
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setDescriptorCount(1)
            .setBinding(DESCRIPTOR_CAMVPUBO_BINDING_ID);
        defaultBinding[1]
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .setDescriptorCount(1)
            .setBinding(DESCRIPTOR_LIGHTUBO_BINDING_ID);
        defaultBinding[2]
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setDescriptorCount(1)
            .setBinding(DESCRIPTOR_MODELUBO_BINDING_ID);
    }
    return defaultBinding;
}

void VulkanUBODescriptorSetLayout::Finish()
{
    if (m_vkDescriptorSetLayout)
    {
        return;
    }
    m_bindings = GetVkBinding();
    static auto layoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);
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
    m_bindings = GetVkBinding();
    static auto layoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);
    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}

const std::vector<vk::DescriptorSetLayoutBinding>& VulkanShadowMapDescriptorSetLayout::GetVkBinding()
{
    static std::vector<vk::DescriptorSetLayoutBinding> bindings;
    if (bindings.empty())
    {
        for (int i = DESCRIPTOR_SHADOWMAP1_BINDING_ID; i <= DESCRIPTOR_SHADOWMAP5_BINDING_ID; i++)
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

void VulkanShadowMapDescriptorSetLayout::Finish()
{
    if (m_vkDescriptorSetLayout)
    {
        return;
    }
    m_bindings = GetVkBinding();
    static auto layoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(m_bindings);
    m_vkDescriptorSetLayout = m_vulkanDevice->GetVkDevice().createDescriptorSetLayout(layoutInfo);
}