#pragma once
#include "Runtime/VulkanRHI/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <memory>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanShaderSet;
class VulkanDevice;
class VulkanRenderPipeline
{
public:
private:
    VulkanShaderSet* m_vulkanShaderSet = nullptr;
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanRenderPipeline* m_parent = nullptr;

    std::unique_ptr<VulkanDynamicState> m_pVulkanDynamicState;
    std::unique_ptr<VulkanDescriptorSets> m_pVulkanDescriptorSets;
    std::unique_ptr<VulkanDescriptorSetLayout> m_pVulkanDescriptorSetLayout;
    std::unique_ptr<VulkanPipelineLayout> m_pVulkanPipelineLayout;
public:
    explicit VulkanRenderPipeline(VulkanDevice* device, VulkanShaderSet* shaderset, VulkanRenderPipeline* input);
    ~VulkanRenderPipeline();
};
RHI_NAMESPACE_END