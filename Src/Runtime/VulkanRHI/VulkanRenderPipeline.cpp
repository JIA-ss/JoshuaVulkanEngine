#include "VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice* device, VulkanShaderSet* shaderset, VulkanRenderPipeline* input)
    : m_vulkanShaderSet(shaderset)
    , m_vulkanDevice(device)
    , m_parent(input)
{
    if (!shaderset)
    {
        m_vulkanShaderSet = m_parent->m_vulkanShaderSet;
    }

    m_pVulkanDynamicState.reset(new VulkanDynamicState(this));
    m_pVulkanDescriptorSets.reset(new VulkanDescriptorSets());
    m_pVulkanDescriptorSetLayout.reset(new VulkanDescriptorSetLayout(m_vulkanDevice, m_vulkanShaderSet));
    m_pVulkanPipelineLayout.reset(new VulkanPipelineLayout(m_vulkanDevice, m_pVulkanDescriptorSetLayout.get()));

}


VulkanRenderPipeline::~VulkanRenderPipeline()
{
    m_pVulkanPipelineLayout.reset();
    m_pVulkanDescriptorSetLayout.reset();
    m_pVulkanDescriptorSets.reset();
    m_pVulkanDynamicState.reset();
}