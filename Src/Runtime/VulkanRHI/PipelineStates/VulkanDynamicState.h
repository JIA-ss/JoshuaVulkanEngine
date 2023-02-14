#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <cmath>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanRenderPipeline;
class VulkanDynamicState
{
public:
private:
    std::vector<vk::DynamicState> m_vkDymanicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
public:
    explicit VulkanDynamicState(std::optional<std::vector<vk::DynamicState>> states = {});
    ~VulkanDynamicState();

    vk::PipelineDynamicStateCreateInfo GetDynamicStateCreateInfo();
    void SetUpCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline);

private:
    void setupViewPortCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline);
    void setupScissorCmdBuf(vk::CommandBuffer& cmd, VulkanRenderPipeline* pipeline);
};
RHI_NAMESPACE_END