#include "VulkanDepthStencilState.h"
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

VulkanDepthStencilState::VulkanDepthStencilState()
{

}


VulkanDepthStencilState::~VulkanDepthStencilState()
{

}

vk::PipelineDepthStencilStateCreateInfo VulkanDepthStencilState::GetDepthStencilStateCreateInfo()
{
    return vk::PipelineDepthStencilStateCreateInfo()
            .setDepthTestEnable(VK_TRUE)
            .setDepthWriteEnable(VK_TRUE)
            .setDepthCompareOp(vk::CompareOp::eLess)
            .setDepthBoundsTestEnable(VK_FALSE)
            .setMinDepthBounds(0.0f)
            .setMaxDepthBounds(1.0f)
            .setStencilTestEnable(VK_FALSE);
}