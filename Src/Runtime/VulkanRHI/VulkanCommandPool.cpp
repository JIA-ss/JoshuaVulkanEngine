#include "VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"
#include "Runtime/VulkanRHI/VulkanDevice.h"

RHI_NAMESPACE_USING

VulkanCommandPool::VulkanCommandPool(VulkanDevice* device, uint32_t queueFamilyIndex)
    : m_pVulkanDevice(device)
{
    auto poolInfo = vk::CommandPoolCreateInfo()
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(queueFamilyIndex);
    m_vkCmdPool = m_pVulkanDevice->GetVkDevice().createCommandPool(poolInfo);
}

VulkanCommandPool::~VulkanCommandPool()
{
    m_pVulkanDevice->GetVkDevice().destroyCommandPool(m_vkCmdPool);
}

vk::CommandBuffer VulkanCommandPool::BeginSingleTimeCommand()
{
    vk::CommandBufferAllocateInfo allocateInfo;
    allocateInfo.setCommandBufferCount(1)
                .setCommandPool(m_vkCmdPool)
                .setLevel(vk::CommandBufferLevel::ePrimary);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    vk::CommandBuffer cmd = m_pVulkanDevice->GetVkDevice().allocateCommandBuffers(allocateInfo).front();
    cmd.begin(beginInfo);
    return cmd;
}

void VulkanCommandPool::EndSingleTimeCommand(vk::CommandBuffer& cmd, vk::Queue& queue)
{
    cmd.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1)
                .setCommandBuffers(cmd);
    queue.submit(submitInfo);
    queue.waitIdle();

    m_pVulkanDevice->GetVkDevice().freeCommandBuffers(m_vkCmdPool, cmd);
}