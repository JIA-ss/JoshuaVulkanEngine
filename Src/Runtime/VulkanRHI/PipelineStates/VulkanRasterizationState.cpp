#include "VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanRasterizationState::VulkanRasterizationState()
{

}


VulkanRasterizationState::~VulkanRasterizationState()
{

}

vk::PipelineRasterizationStateCreateInfo VulkanRasterizationState::GetRasterizationStateCreateInfo()
{
    auto createInfo = vk::PipelineRasterizationStateCreateInfo()
                    .setDepthClampEnable(false)
                    .setRasterizerDiscardEnable(false)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setLineWidth(1.0f)
                    .setCullMode(vk::CullModeFlagBits::eBack)
                    .setFrontFace(vk::FrontFace::eCounterClockwise)
                    .setDepthBiasEnable(false);
    return createInfo;
}