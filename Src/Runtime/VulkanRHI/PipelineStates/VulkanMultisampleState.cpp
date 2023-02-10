#include "VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanMultisampleState::VulkanMultisampleState(vk::SampleCountFlagBits sampleCount)
    : m_sampleCount(sampleCount)
{

}


VulkanMultisampleState::~VulkanMultisampleState()
{

}


vk::PipelineMultisampleStateCreateInfo VulkanMultisampleState::GetMultisampleStateCreateInfo()
{
    auto createInfo = vk::PipelineMultisampleStateCreateInfo()
                    .setSampleShadingEnable(false)
                    .setRasterizationSamples(m_sampleCount);
    return createInfo;
}