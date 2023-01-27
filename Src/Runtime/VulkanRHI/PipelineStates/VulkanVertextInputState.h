#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanVertextInputState
{
public:
private:
public:
    VulkanVertextInputState();
    ~VulkanVertextInputState();

    vk::PipelineVertexInputStateCreateInfo GetVertexInputStateCreateInfo();
};
RHI_NAMESPACE_END