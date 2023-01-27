#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanMultisampleState
{
public:
private:
public:
    VulkanMultisampleState();
    ~VulkanMultisampleState();

    vk::PipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo();
};
RHI_NAMESPACE_END   