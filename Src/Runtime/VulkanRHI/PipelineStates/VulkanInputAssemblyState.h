#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanInputAssemblyState
{
public:
private:
public:
    VulkanInputAssemblyState();
    ~VulkanInputAssemblyState();

    vk::PipelineInputAssemblyStateCreateInfo GetIputAssemblyStaeCreateInfo();
};
RHI_NAMESPACE_END