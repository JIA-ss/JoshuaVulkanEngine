#include "VulkanInputAssemblyState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanInputAssemblyState::VulkanInputAssemblyState()
{

}


VulkanInputAssemblyState::~VulkanInputAssemblyState()
{

}

vk::PipelineInputAssemblyStateCreateInfo VulkanInputAssemblyState::GetIputAssemblyStaeCreateInfo()
{
    vk::PipelineInputAssemblyStateCreateInfo createInfo;
    createInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
                .setPrimitiveRestartEnable(false);
    return createInfo;
}