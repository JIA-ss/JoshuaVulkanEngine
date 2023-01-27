#include "VulkanViewportState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanViewportState::VulkanViewportState(const std::vector<vk::Viewport>& viewports, const std::vector<vk::Rect2D>& scissors)
    : m_vkViewports(viewports)
    , m_vkScissors(scissors)
{
    m_vkCreateInfo.setViewports(m_vkViewports)
                .setScissors(m_vkScissors);
}


VulkanViewportState::~VulkanViewportState()
{

}

vk::PipelineViewportStateCreateInfo VulkanViewportState::GetViewportStateCreateInfo()
{
    return m_vkCreateInfo;
}