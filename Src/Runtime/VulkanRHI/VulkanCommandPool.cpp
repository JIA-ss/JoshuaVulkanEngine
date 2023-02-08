#include "VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
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

vk::CommandBuffer VulkanCommandPool::CreateReUsableCmd()
{
    vk::CommandBufferAllocateInfo allocateInfo;
    allocateInfo.setCommandBufferCount(1)
                .setCommandPool(m_vkCmdPool)
                .setLevel(vk::CommandBufferLevel::ePrimary);
    return m_pVulkanDevice->GetVkDevice().allocateCommandBuffers(allocateInfo).front();
}

vk::CommandBuffer VulkanCommandPool::BeginSingleTimeCommand()
{
    auto allocInfo = vk::CommandBufferAllocateInfo()
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandPool(m_vkCmdPool)
                .setCommandBufferCount(1);
    vk::CommandBuffer buf = m_pVulkanDevice->GetVkDevice().allocateCommandBuffers(allocInfo).front();

    auto beginInfo = vk::CommandBufferBeginInfo()
                .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    buf.begin(beginInfo);
    return buf;
}

void VulkanCommandPool::EndSingleTimeCommand(vk::CommandBuffer cmd, vk::Queue queue)
{
    cmd.end();

    auto submitInfo = vk::SubmitInfo()
                .setCommandBufferCount(1)
                .setCommandBuffers(cmd);
    queue.submit(submitInfo);
    queue.waitIdle();

    m_pVulkanDevice->GetVkDevice().freeCommandBuffers(m_vkCmdPool, cmd);
}