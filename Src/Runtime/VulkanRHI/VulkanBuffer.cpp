#include "VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanBuffer::VulkanBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode)
    : m_vulkanDevice(device)
    , m_vkSize(size)
    , m_vkBufUsage(usage)
    , m_vkMemProps(properties)
    , m_vkSharingMode(sharingMode)
{
    auto bufferInfo = vk::BufferCreateInfo()
                    .setSize(m_vkSize)
                    .setUsage(m_vkBufUsage)
                    .setSharingMode(m_vkSharingMode);
    
    m_vkBuf = m_vulkanDevice->GetVkDevice().createBuffer(bufferInfo);

    vk::MemoryRequirements requirement = m_vulkanDevice->GetVkDevice().getBufferMemoryRequirements(m_vkBuf);

    auto memoryAllocInfo = vk::MemoryAllocateInfo()
                        .setAllocationSize(requirement.size)
                        .setMemoryTypeIndex(findMemoryType(requirement.memoryTypeBits, m_vkMemProps))
                        ;
    m_vkDeviceMemory = m_vulkanDevice->GetVkDevice().allocateMemory(memoryAllocInfo);

    m_vulkanDevice->GetVkDevice().bindBufferMemory(m_vkBuf, m_vkDeviceMemory, 0);
}


VulkanBuffer::~VulkanBuffer()
{
    destroy();
}

void VulkanBuffer::destroy()
{
    if (m_vkBuf)
    {
        m_vulkanDevice->GetVkDevice().destroyBuffer(m_vkBuf);
    }
    if (m_vkDeviceMemory)
    {
        m_vulkanDevice->GetVkDevice().freeMemory(m_vkDeviceMemory);
    }
}

uint32_t VulkanBuffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    auto& memoryProps = m_vulkanDevice->GetVulkanPhysicalDevice()->GetVkPhysicalDeviceMemoryProps();

    for (int i = 0; i < memoryProps.memoryTypeCount; i++)
    {
        if (
            (typeFilter & (1 << i))
            &&
            ((memoryProps.memoryTypes[i].propertyFlags & properties) == properties)
        )
        {
            return i;
        }
    }

    throw std::runtime_error("faild to find suitable memory type");
    return 0;
}


void VulkanBuffer::FillingBuffer(void* data, std::size_t offset, std::size_t size)
{
    assert(offset + size <= m_vkSize);
    assert(m_vkMemProps & vk::MemoryPropertyFlagBits::eHostVisible);
    assert(m_vkMemProps & vk::MemoryPropertyFlagBits::eHostCoherent);

    void* dstData = m_vulkanDevice->GetVkDevice().mapMemory(m_vkDeviceMemory, offset, size);
    memcpy(dstData, data, size);
    //m_vulkanDevice->GetVkDevice().flushMappedMemoryRanges(...);
    m_vulkanDevice->GetVkDevice().unmapMemory(m_vkDeviceMemory);
}


VulkanGPUBuffer::VulkanGPUBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
    : VulkanBuffer(device, size, usage, props, sharingMode)
{
    m_pVulkanCPUBuffer.reset(new VulkanBuffer(device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, sharingMode));
}

VulkanGPUBuffer::~VulkanGPUBuffer() 
{
    m_pVulkanCPUBuffer.reset();
}



void VulkanGPUBuffer::FillingBuffer(void* data, std::size_t size, std::size_t offset)
{
    m_pVulkanCPUBuffer->FillingBuffer(data, offset, size);
}

void VulkanGPUBuffer::CopyDataToGPU(vk::CommandBuffer cmd, vk::Queue queue, std::size_t size, std::size_t dstOffset, std::size_t srcOffset)
{
    auto beginInfo = vk::CommandBufferBeginInfo()
                    .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(beginInfo);
    {
        vk::BufferCopy copy;
        copy.setSize(size)
            .setDstOffset(dstOffset)
            .setSrcOffset(srcOffset);
        cmd.copyBuffer(*m_pVulkanCPUBuffer->GetPVkBuf(), m_vkBuf, copy);
    }
    cmd.end();

    auto submitInfo = vk::SubmitInfo()
                    .setCommandBuffers(cmd);
    queue.submit(submitInfo);
    queue.waitIdle();
}