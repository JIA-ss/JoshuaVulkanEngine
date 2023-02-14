#include "VulkanDynamicState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDynamicState::VulkanDynamicState(std::optional<std::vector<vk::DynamicState>> states)
{
    if (states.has_value())
    {
        m_vkDymanicStates = states.value();
    }
}


VulkanDynamicState::~VulkanDynamicState()
{

}



vk::PipelineDynamicStateCreateInfo VulkanDynamicState::GetDynamicStateCreateInfo()
{
    vk::PipelineDynamicStateCreateInfo createInfo;
    createInfo.setDynamicStates(m_vkDymanicStates);
    return createInfo;
}

void VulkanDynamicState::SetUpCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline)
{
    for (vk::DynamicState& state: m_vkDymanicStates)
    {
        switch (state)
        {
        case vk::DynamicState::eViewport:
        {
            setupViewPortCmdBuf(cmd, pipeline);
            break;
        }
        case vk::DynamicState::eScissor:
        {
            setupScissorCmdBuf(cmd, pipeline);
            break;
        }
        default:
            assert(false);
        }
    }
}


void VulkanDynamicState::setupViewPortCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline)
{
    auto& extent = pipeline->GetPVulkanDevice()->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
    cmd.setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
}

void VulkanDynamicState::setupScissorCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline)
{

    auto& extent = pipeline->GetPVulkanDevice()->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
    vk::Rect2D rect{{0,0},extent};
    cmd.setScissor(0,rect);
}