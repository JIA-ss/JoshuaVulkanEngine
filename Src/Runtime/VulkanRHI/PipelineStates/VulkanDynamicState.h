#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vector>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanRenderPipeline;
class VulkanDynamicState
{
public:
private:
    VulkanRenderPipeline* m_vulkanRenderPipeline = nullptr;
    std::vector<vk::DynamicState> m_vkDymanicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
public:
    explicit VulkanDynamicState(VulkanRenderPipeline* pipeline);
    ~VulkanDynamicState();

    vk::PipelineDynamicStateCreateInfo GetDynamicStateCreateInfo();
    void SetUpCmdBuf(vk::CommandBuffer& cmd);

private:
    void setupViewPortCmdBuf(vk::CommandBuffer& cmd);
    void setupScissorCmdBuf(vk::CommandBuffer& cmd);
};
RHI_NAMESPACE_END