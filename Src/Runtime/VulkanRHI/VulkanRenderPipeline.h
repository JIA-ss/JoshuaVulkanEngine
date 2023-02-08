#pragma once
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDepthStencilState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanInputAssemblyState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanVertextInputState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanViewportState.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"
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
    VulkanDescriptorSetLayout* m_vulkanDescSetLayout = nullptr;
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanRenderPipeline* m_parent = nullptr;

    std::unique_ptr<VulkanDynamicState> m_pVulkanDynamicState;
    std::unique_ptr<VulkanInputAssemblyState> m_pVulkanInputAssemblyState;
    std::unique_ptr<VulkanVertextInputState> m_pVulkanVertexInputState;
    std::unique_ptr<VulkanViewportState> m_pVulkanViewPortState;
    std::unique_ptr<VulkanRasterizationState> m_pVulkanRasterizationState;
    std::unique_ptr<VulkanMultisampleState> m_pVulkanMultisampleState;
    std::unique_ptr<VulkanDepthStencilState> m_pVulkanDepthStencilState;
    std::unique_ptr<VulkanColorBlendState> m_pVulkanColorBlendState;

    std::unique_ptr<VulkanRenderPass> m_pVulkanRenderPass;

    std::unique_ptr<VulkanPipelineLayout> m_pVulkanPipelineLayout;

    vk::Pipeline m_vkPipeline;
public:
    explicit VulkanRenderPipeline(VulkanDevice* device, VulkanShaderSet* shaderset, VulkanDescriptorSetLayout* layout, VulkanRenderPipeline* input);
    ~VulkanRenderPipeline();

    inline VulkanDynamicState* GetPVulkanDynamicState() { return m_pVulkanDynamicState.get(); }
    inline VulkanDevice* GetPVulkanDevice() { return m_vulkanDevice; }
    inline vk::Pipeline& GetVkPipeline() { return m_vkPipeline; }
    inline vk::RenderPass& GetVkRenderPass() { return m_pVulkanRenderPass->GetVkRenderPass(); }
    inline vk::PipelineLayout& GetVkPipelineLayout() { return m_pVulkanPipelineLayout->GetVkPieplineLayout(); }
};
RHI_NAMESPACE_END