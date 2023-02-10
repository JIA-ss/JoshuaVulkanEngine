#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanMultisampleState
{
public:
private:
    vk::SampleCountFlagBits m_sampleCount;
public:
    VulkanMultisampleState(vk::SampleCountFlagBits sampleCount);
    ~VulkanMultisampleState();

    vk::PipelineMultisampleStateCreateInfo GetMultisampleStateCreateInfo();
};
RHI_NAMESPACE_END   