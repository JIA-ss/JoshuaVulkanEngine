#include "VulkanVertextInputState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanVertextInputState::VulkanVertextInputState()
{

}


VulkanVertextInputState::~VulkanVertextInputState()
{

}

vk::PipelineVertexInputStateCreateInfo VulkanVertextInputState::GetVertexInputStateCreateInfo()
{
    vk::PipelineVertexInputStateCreateInfo createInfo;
    createInfo.setVertexBindingDescriptionCount(0)
                .setVertexAttributeDescriptionCount(0);
    return createInfo;
}
