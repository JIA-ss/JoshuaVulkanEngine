#pragma once
#include "Runtime/VulkanRHI/Graphic/Material.h"
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
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "vulkan/vulkan_handles.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanShaderSet;
class VulkanDevice;

class VulkanRenderPipeline
{
public:
protected:
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanRenderPipeline* m_parent;

    std::shared_ptr<VulkanShaderSet> m_vulkanShaderSet = nullptr;

    std::shared_ptr<VulkanDynamicState> m_pVulkanDynamicState;
    std::shared_ptr<VulkanInputAssemblyState> m_pVulkanInputAssemblyState;
    std::shared_ptr<VulkanVertextInputState> m_pVulkanVertexInputState;
    std::shared_ptr<VulkanViewportState> m_pVulkanViewPortState;
    std::shared_ptr<VulkanRasterizationState> m_pVulkanRasterizationState;
    std::shared_ptr<VulkanMultisampleState> m_pVulkanMultisampleState;
    std::shared_ptr<VulkanDepthStencilState> m_pVulkanDepthStencilState;
    std::shared_ptr<VulkanColorBlendState> m_pVulkanColorBlendState;
    std::shared_ptr<VulkanRenderPass> m_pVulkanRenderPass;
    std::shared_ptr<VulkanPipelineLayout> m_pVulkanPipelineLayout;

    vk::Pipeline m_vkPipeline;

    VulkanRenderPipeline(VulkanDevice* device) : m_vulkanDevice(device) {}
public:
    explicit VulkanRenderPipeline(
        VulkanDevice* device,
        std::shared_ptr<VulkanShaderSet> shaderset,
        VulkanRenderPipeline* input = nullptr,
        std::shared_ptr<VulkanRenderPass> renderpass = nullptr,
        std::shared_ptr<VulkanDynamicState> dynamicState= nullptr,
        std::shared_ptr<VulkanInputAssemblyState> inputAssemblyState= nullptr,
        std::shared_ptr<VulkanVertextInputState> vertexInputState= nullptr,
        std::shared_ptr<VulkanViewportState> viewPortState= nullptr,
        std::shared_ptr<VulkanRasterizationState> rasterizationState= nullptr,
        std::shared_ptr<VulkanMultisampleState> multisampleState= nullptr,
        std::shared_ptr<VulkanDepthStencilState> depthStencilState= nullptr,
        std::shared_ptr<VulkanColorBlendState> blendState= nullptr,
        std::shared_ptr<VulkanPipelineLayout> pipelineLayout = nullptr
        );
    ~VulkanRenderPipeline();

    inline std::shared_ptr<VulkanPipelineLayout> GetVulkanPipelineLayout() { return m_pVulkanPipelineLayout; }
    inline VulkanDynamicState* GetPVulkanDynamicState() { return m_pVulkanDynamicState.get(); }
    inline VulkanDevice* GetPVulkanDevice() { return m_vulkanDevice; }
    inline std::shared_ptr<VulkanRenderPass> GetVulkanRenderPass() { return m_pVulkanRenderPass; }
    inline vk::Pipeline& GetVkPipeline() { return m_vkPipeline; }
    inline vk::RenderPass& GetVkRenderPass() { return m_pVulkanRenderPass->GetVkRenderPass(); }
    inline vk::PipelineLayout& GetVkPipelineLayout() { return m_pVulkanPipelineLayout->GetVkPieplineLayout(); }
};



class VulkanRenderPipelineBuilder
{
protected:
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanRenderPipeline* m_parent;

    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_descriptorSetLayouts;

    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanShaderSet, shaderSet)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanDynamicState, VulkanDynamicState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanInputAssemblyState, VulkanInputAssemblyState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanVertextInputState, VulkanVertexInputState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanViewportState, VulkanViewPortState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanRasterizationState, VulkanRasterizationState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanMultisampleState, VulkanMultisampleState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanDepthStencilState, VulkanDepthStencilState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanColorBlendState, VulkanColorBlendState)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanRenderPass, VulkanRenderPass)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanPipelineLayout, VulkanPipelineLayout)
public:
    explicit VulkanRenderPipelineBuilder(VulkanDevice* device, VulkanRenderPipeline* parent = nullptr)
        : m_vulkanDevice(device)
        , m_parent(parent)
    {}

    VulkanRenderPipelineBuilder& AddDescriptorLayout(std::shared_ptr<VulkanDescriptorSetLayout> layout);
    VulkanRenderPipelineBuilder& SetDescriptorLayout(std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> layouts);
    VulkanRenderPipeline* build();
    std::shared_ptr<VulkanRenderPipeline> buildShared();
    std::unique_ptr<VulkanRenderPipeline> buildUnique();
};



RHI_NAMESPACE_END