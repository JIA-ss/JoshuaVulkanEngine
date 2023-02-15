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
private:
    VulkanDevice* m_vulkanDevice = nullptr;
    std::weak_ptr<VulkanRenderPipeline> m_parent;

    std::shared_ptr<VulkanShaderSet> m_vulkanShaderSet = nullptr;
    std::shared_ptr<VulkanDescriptorSetLayout> m_vulkanDescSetLayout = nullptr;

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
public:
    explicit VulkanRenderPipeline(
        VulkanDevice* device,
        std::shared_ptr<VulkanShaderSet> shaderset,
        std::shared_ptr<VulkanDescriptorSetLayout> layout,
        std::shared_ptr<VulkanRenderPipeline> input = nullptr,
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

    inline VulkanDynamicState* GetPVulkanDynamicState() { return m_pVulkanDynamicState.get(); }
    inline VulkanDevice* GetPVulkanDevice() { return m_vulkanDevice; }
    inline std::shared_ptr<VulkanRenderPass> GetVulkanRenderPass() { return m_pVulkanRenderPass; }
    inline vk::Pipeline& GetVkPipeline() { return m_vkPipeline; }
    inline vk::RenderPass& GetVkRenderPass() { return m_pVulkanRenderPass->GetVkRenderPass(); }
    inline vk::PipelineLayout& GetVkPipelineLayout() { return m_pVulkanPipelineLayout->GetVkPieplineLayout(); }
};


#define BUILDER_SHARED_PTR_SET_FUNC(_BUILDER_TYPE_, _PROP_TYPE_, _PROP_NAME_)   \
private:    \
    std::shared_ptr<_PROP_TYPE_> m_##_PROP_NAME_ = nullptr; \
public: \
    inline _BUILDER_TYPE_& Set##_PROP_NAME_(std::shared_ptr<_PROP_TYPE_> prop) { m_##_PROP_NAME_ = prop; return *this; }

class VulkanRenderPipelineBuilder
{
private:
    VulkanDevice* m_vulkanDevice = nullptr;
    std::weak_ptr<VulkanRenderPipeline> m_parent;

    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanShaderSet, shaderSet)
    BUILDER_SHARED_PTR_SET_FUNC(VulkanRenderPipelineBuilder, VulkanDescriptorSetLayout, descriptorSetLayout)
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
    explicit VulkanRenderPipelineBuilder(VulkanDevice* device, std::shared_ptr<VulkanRenderPipeline> parent = nullptr)
        : m_vulkanDevice(device)
        , m_parent(parent)
    {}

    std::shared_ptr<VulkanRenderPipeline> build();
};

#undef BUILDER_SHARED_PTR_SET_FUNC

RHI_NAMESPACE_END