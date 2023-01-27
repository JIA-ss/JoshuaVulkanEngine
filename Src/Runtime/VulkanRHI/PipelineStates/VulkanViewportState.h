#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vector>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanViewportState
{
public:
private:
    std::vector<vk::Viewport> m_vkViewports;
    std::vector<vk::Rect2D> m_vkScissors;
    vk::PipelineViewportStateCreateInfo m_vkCreateInfo;
public:
    explicit VulkanViewportState(const std::vector<vk::Viewport>& viewports,const std::vector<vk::Rect2D>& scissor);
    ~VulkanViewportState();

    vk::PipelineViewportStateCreateInfo GetViewportStateCreateInfo();
};
RHI_NAMESPACE_END