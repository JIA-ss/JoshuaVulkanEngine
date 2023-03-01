#include "VulkanDepthStencilState.h"
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

const VulkanDepthStencilState::Config VulkanDepthStencilState::Config::DEFAULT = {};

VulkanDepthStencilState::VulkanDepthStencilState(const Config& config)
    : m_config(config)
{

}


VulkanDepthStencilState::~VulkanDepthStencilState()
{

}

vk::PipelineDepthStencilStateCreateInfo VulkanDepthStencilState::GetDepthStencilStateCreateInfo()
{
    return vk::PipelineDepthStencilStateCreateInfo()
            .setDepthTestEnable(m_config.DepthTestEnable)
            .setDepthWriteEnable(m_config.DepthWriteEnable)
            .setDepthCompareOp(m_config.DepthCompareOp)
            .setDepthBoundsTestEnable(m_config.DepthBoundsTestEnable)
            .setMinDepthBounds(m_config.MinDepthBounds)
            .setMaxDepthBounds(m_config.MaxDepthBounds)
            .setStencilTestEnable(m_config.StencilTestEnable);
}