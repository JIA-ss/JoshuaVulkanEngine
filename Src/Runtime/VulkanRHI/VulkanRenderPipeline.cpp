#include "VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDepthStencilState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanInputAssemblyState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanVertextInputState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanViewportState.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanRenderPipelineBuilder& VulkanRenderPipelineBuilder::AddDescriptorLayout(std::shared_ptr<VulkanDescriptorSetLayout> layout)
{
    m_descriptorSetLayouts.push_back(layout);
    return *this;
}

VulkanRenderPipelineBuilder& VulkanRenderPipelineBuilder::SetDescriptorLayout(std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> layouts)
{
    m_descriptorSetLayouts.clear();
    for (int i = 0; i < layouts.size(); i++)
    {
        AddDescriptorLayout(layouts[i]);
    }
    return *this;
}

VulkanRenderPipeline* VulkanRenderPipelineBuilder::build()
{
    assert(m_vulkanDevice);
    int windowWidth = m_vulkanDevice->GetVulkanPhysicalDevice()->GetWindowWidth();
    int windowHeight = m_vulkanDevice->GetVulkanPhysicalDevice()->GetWindowHeight();

    vk::Viewport viewport{0,0,(float)windowWidth, (float)windowHeight, 0,0};
    vk::Rect2D scissor{vk::Offset2D{0,0}, vk::Extent2D{(uint32_t)windowWidth, (uint32_t)windowHeight}};

    vk::Format depthForamt = m_vulkanDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    vk::SampleCountFlagBits sampleCount = m_vulkanDevice->GetVulkanPhysicalDevice()->GetSampleCount();
    if (!m_VulkanDynamicState) m_VulkanDynamicState.reset(new VulkanDynamicState());
    if (!m_VulkanInputAssemblyState) m_VulkanInputAssemblyState.reset(new VulkanInputAssemblyState());
    if (!m_VulkanVertexInputState) m_VulkanVertexInputState.reset(new VulkanVertextInputState());
    if (!m_VulkanViewPortState) m_VulkanViewPortState.reset(new VulkanViewportState({viewport}, {scissor}));
    if (!m_VulkanRasterizationState) m_VulkanRasterizationState = VulkanRasterizationStateBuilder().build();
    if (!m_VulkanMultisampleState) m_VulkanMultisampleState.reset(new VulkanMultisampleState(sampleCount));
    if (!m_VulkanDepthStencilState) m_VulkanDepthStencilState.reset(new VulkanDepthStencilState());
    if (!m_VulkanColorBlendState) m_VulkanColorBlendState.reset(new VulkanColorBlendState());
    assert(m_VulkanRenderPass); // m_VulkanRenderPass.reset(new VulkanRenderPass(m_vulkanDevice, m_vulkanDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format, depthForamt, sampleCount));
    if (!m_VulkanPipelineLayout) m_VulkanPipelineLayout.reset(new VulkanPipelineLayout(m_vulkanDevice, m_descriptorSetLayouts));

    return new VulkanRenderPipeline(
        m_vulkanDevice,
        m_shaderSet,
        m_parent,
        m_VulkanRenderPass,
        m_VulkanDynamicState,
        m_VulkanInputAssemblyState,
        m_VulkanVertexInputState,
        m_VulkanViewPortState,
        m_VulkanRasterizationState,
        m_VulkanMultisampleState,
        m_VulkanDepthStencilState,
        m_VulkanColorBlendState,
        m_VulkanPipelineLayout
    );
}

std::shared_ptr<VulkanRenderPipeline> VulkanRenderPipelineBuilder::buildShared()
{
    std::shared_ptr<VulkanRenderPipeline> pipeline;
    pipeline.reset(build());
    return pipeline;
}
std::unique_ptr<VulkanRenderPipeline> VulkanRenderPipelineBuilder::buildUnique()
{
    std::unique_ptr<VulkanRenderPipeline> pipeline;
    pipeline.reset(build());
    return pipeline;
}

VulkanRenderPipeline::VulkanRenderPipeline(
    VulkanDevice* device,
    std::shared_ptr<VulkanShaderSet> shaderset,
    VulkanRenderPipeline* input,
    std::shared_ptr<VulkanRenderPass> renderpass,
    std::shared_ptr<VulkanDynamicState> dynamicState,
    std::shared_ptr<VulkanInputAssemblyState> inputAssemblyState,
    std::shared_ptr<VulkanVertextInputState> vertexInputState,
    std::shared_ptr<VulkanViewportState> viewPortState,
    std::shared_ptr<VulkanRasterizationState> rasterizationState,
    std::shared_ptr<VulkanMultisampleState> multisampleState,
    std::shared_ptr<VulkanDepthStencilState> depthStencilState,
    std::shared_ptr<VulkanColorBlendState> blendState,
    std::shared_ptr<VulkanPipelineLayout> pipelineLayout
)
    : m_vulkanDevice(device)
    , m_vulkanShaderSet(shaderset)
    , m_parent(input)
    , m_pVulkanRenderPass(renderpass)
    , m_pVulkanDynamicState(dynamicState)
    , m_pVulkanInputAssemblyState(inputAssemblyState)
    , m_pVulkanVertexInputState(vertexInputState)
    , m_pVulkanViewPortState(viewPortState)
    , m_pVulkanRasterizationState(rasterizationState)
    , m_pVulkanMultisampleState(multisampleState)
    , m_pVulkanDepthStencilState(depthStencilState)
    , m_pVulkanColorBlendState(blendState)
    , m_pVulkanPipelineLayout(pipelineLayout)
{
    if (!shaderset && m_parent)
    {
        m_vulkanShaderSet = m_parent->m_vulkanShaderSet;
    }

    vk::GraphicsPipelineCreateInfo createInfo;
    auto dynamicInfo = m_pVulkanDynamicState->GetDynamicStateCreateInfo();
    auto assemblyInfo = m_pVulkanInputAssemblyState->GetIputAssemblyStaeCreateInfo();
    auto vertexInputInfo = m_pVulkanVertexInputState->GetVertexInputStateCreateInfo();
    auto viewportInfo = m_pVulkanViewPortState->GetViewportStateCreateInfo();
    auto rasterizationInfo = m_pVulkanRasterizationState->GetRasterizationStateCreateInfo();
    auto multisampleInfo = m_pVulkanMultisampleState->GetMultisampleStateCreateInfo();
    auto depthStencilInfo = m_pVulkanDepthStencilState->GetDepthStencilStateCreateInfo();
    auto colorBlendingInfo = m_pVulkanColorBlendState->GetColorBlendStateCreateInfo();
    auto shaderStages = m_vulkanShaderSet->GetShaderCreateInfos();
    createInfo.setPDynamicState(&dynamicInfo)
                // vertex input
                .setPVertexInputState(&vertexInputInfo)
                // vertex assembly
                .setPInputAssemblyState(&assemblyInfo)
                // shader
                .setStages(shaderStages)
                // viewport
                .setPViewportState(&viewportInfo)
                // rasterization
                .setPRasterizationState(&rasterizationInfo)
                // multisample
                .setPMultisampleState(&multisampleInfo)
                // depth test
                .setPDepthStencilState(&depthStencilInfo)
                // color blending
                .setPColorBlendState(&colorBlendingInfo)
                // layout
                .setLayout(m_pVulkanPipelineLayout->GetVkPieplineLayout())
                // render pass
                .setRenderPass(m_pVulkanRenderPass.lock()->GetVkRenderPass());
    if (m_parent)
    {
        createInfo.setFlags(vk::PipelineCreateFlagBits::eDerivative)
                    .setBasePipelineHandle(m_parent->GetVkPipeline())
                    .setBasePipelineIndex(-1);
    }
    else
    {
        createInfo.setFlags(vk::PipelineCreateFlagBits::eAllowDerivatives);
    }

    auto result = m_vulkanDevice->GetVkDevice().createGraphicsPipeline(nullptr, createInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("create graphics pipeline failed");
    }
    m_vkPipeline = result.value;
}


VulkanRenderPipeline::~VulkanRenderPipeline()
{
    m_pVulkanPipelineLayout.reset();
    m_pVulkanDynamicState.reset();
    m_pVulkanInputAssemblyState.reset();
    m_pVulkanVertexInputState.reset();
    m_pVulkanViewPortState.reset();
    m_pVulkanRasterizationState.reset();
    m_pVulkanMultisampleState.reset();
    m_pVulkanColorBlendState.reset();
    m_vulkanShaderSet.reset();
    m_vulkanDevice->GetVkDevice().destroyPipeline(m_vkPipeline);   
}