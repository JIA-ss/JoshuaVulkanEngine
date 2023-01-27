#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanRasterizationState
{
public:
private:
public:
    VulkanRasterizationState();
    ~VulkanRasterizationState();

    vk::PipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo();
};
RHI_NAMESPACE_END