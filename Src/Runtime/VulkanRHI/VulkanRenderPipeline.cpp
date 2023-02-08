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
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice* device, VulkanShaderSet* shaderset,VulkanDescriptorSetLayout* layout, VulkanRenderPipeline* input)
    : m_vulkanShaderSet(shaderset)
    , m_vulkanDescSetLayout(layout)
    , m_vulkanDevice(device)
    , m_parent(input)
{
    if (!shaderset)
    {
        m_vulkanShaderSet = m_parent->m_vulkanShaderSet;
    }

    int windowWidth = device->GetVulkanPhysicalDevice()->GetWindowWidth();
    int windowHeight = device->GetVulkanPhysicalDevice()->GetWindowHeight();
    

    vk::Viewport viewport{0,0,(float)windowWidth, (float)windowHeight, 0,0};
    vk::Rect2D scissor{vk::Offset2D{0,0}, vk::Extent2D{(uint32_t)windowWidth, (uint32_t)windowHeight}};

    vk::Format depthForamt = device->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();

    m_pVulkanDynamicState.reset(new VulkanDynamicState(this));
    m_pVulkanInputAssemblyState.reset(new VulkanInputAssemblyState());
    m_pVulkanVertexInputState.reset(new VulkanVertextInputState());
    m_pVulkanViewPortState.reset(new VulkanViewportState({viewport}, {scissor}));
    m_pVulkanRasterizationState.reset(new VulkanRasterizationState());
    m_pVulkanMultisampleState.reset(new VulkanMultisampleState());
    m_pVulkanDepthStencilState.reset(new VulkanDepthStencilState());
    m_pVulkanColorBlendState.reset(new VulkanColorBlendState());

    m_pVulkanRenderPass.reset(new VulkanRenderPass(device, device->GetPVulkanSwapchain()->GetSwapchainInfo().format.format, depthForamt, vk::SampleCountFlagBits::e1));

    m_pVulkanPipelineLayout.reset(new VulkanPipelineLayout(m_vulkanDevice, m_vulkanDescSetLayout));


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
                .setRenderPass(m_pVulkanRenderPass->GetVkRenderPass());
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

    m_vulkanDevice->GetVkDevice().destroyPipeline(m_vkPipeline);   
}