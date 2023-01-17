#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanRenderPipeline;
class VulkanDynamicState
{
public:
private:
    VulkanRenderPipeline* m_vulkanRenderPipeline = nullptr;
public:
    explicit VulkanDynamicState(VulkanRenderPipeline* pipeline);
    ~VulkanDynamicState();
};
RHI_NAMESPACE_END