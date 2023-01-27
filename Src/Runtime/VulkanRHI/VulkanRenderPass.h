#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanRenderPass
{
public:
private:
    VulkanDevice* m_vulkanDevice;

    vk::RenderPass m_vkRenderPass;
    vk::Format m_colorFormat;
    vk::Format m_depthFormat;
public:
    VulkanRenderPass(VulkanDevice* device, vk::Format colorFormat, vk::Format depthFormat, vk::SampleCountFlagBits sample);
    ~VulkanRenderPass();

    inline vk::RenderPass& GetVkRenderPass() { return m_vkRenderPass; }
};
RHI_NAMESPACE_END