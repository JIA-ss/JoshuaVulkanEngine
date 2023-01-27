#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanColorBlendState
{
public:
private:
    vk::PipelineColorBlendAttachmentState m_vkColorBlendAttachmentState;
public:
    VulkanColorBlendState();
    ~VulkanColorBlendState();

    vk::PipelineColorBlendStateCreateInfo GetColorBlendStateCreateInfo();
};
RHI_NAMESPACE_END