#pragma once

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>
RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanCommandPool
{
public:
private:
    vk::CommandPool m_vkCmdPool;
    VulkanDevice* m_pVulkanDevice;
public:
    explicit VulkanCommandPool(VulkanDevice* device, uint32_t queueFamilyIndex);
    ~VulkanCommandPool();

    vk::CommandBuffer CreateReUsableCmd();
};

RHI_NAMESPACE_END