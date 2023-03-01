#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanColorBlendState
{
public:
private:
    std::vector<vk::PipelineColorBlendAttachmentState> m_vkColorBlendAttachmentStates;
public:
    VulkanColorBlendState(const std::vector<vk::PipelineColorBlendAttachmentState>& states) : m_vkColorBlendAttachmentStates(states) { }
    VulkanColorBlendState();
    ~VulkanColorBlendState();

    vk::PipelineColorBlendStateCreateInfo GetColorBlendStateCreateInfo();
};
RHI_NAMESPACE_END