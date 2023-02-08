#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanDepthBuffer
{
public:
private:
    VulkanDevice* m_vulkanDevice;
public:
    VulkanDepthBuffer();
    ~VulkanDepthBuffer();
};
RHI_NAMESPACE_END