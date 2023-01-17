#include "VulkanDynamicState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDynamicState::VulkanDynamicState(VulkanRenderPipeline* pipeline)
    : m_vulkanRenderPipeline(pipeline)
{

}


VulkanDynamicState::~VulkanDynamicState()
{

}