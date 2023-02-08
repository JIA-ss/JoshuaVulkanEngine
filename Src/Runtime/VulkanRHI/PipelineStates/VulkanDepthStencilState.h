#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_BEGIN

class VulkanDepthStencilState
{
public:
private:
public:
    VulkanDepthStencilState();
    ~VulkanDepthStencilState();

    vk::PipelineDepthStencilStateCreateInfo GetDepthStencilStateCreateInfo();
};
RHI_NAMESPACE_END