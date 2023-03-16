#include "VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanDeviceMemory::VulkanDeviceMemory(VulkanDevice* device, vk::MemoryRequirements requirements, vk::MemoryPropertyFlags props)
    : m_vulkanDevice(device)
    , m_vkMemProps(props)
    , m_vkMemRequirements(requirements)
{
    ZoneScopedN("VulkanDeviceMemory::VulkanDeviceMemory");
    auto memoryAllocInfo = vk::MemoryAllocateInfo()
                        .setAllocationSize(m_vkMemRequirements.size)
                        .setMemoryTypeIndex(findMemoryType())
                        ;
    m_vkDeviceMemory = m_vulkanDevice->GetVkDevice().allocateMemory(memoryAllocInfo);
}

VulkanDeviceMemory::~VulkanDeviceMemory()
{
    ZoneScopedN("VulkanDeviceMemory::~VulkanDeviceMemory");
    if (m_vkDeviceMemory)
    {
        m_vulkanDevice->GetVkDevice().freeMemory(m_vkDeviceMemory);
    }
}

void* VulkanDeviceMemory::MapMemory(std::size_t offset, std::size_t size)
{
    ZoneScopedN("VulkanDeviceMemory::MapMemory");
    assert(offset + size <= m_vkMemRequirements.size);
    assert(m_vkMemProps & vk::MemoryPropertyFlagBits::eHostVisible);
    assert(m_vkMemProps & vk::MemoryPropertyFlagBits::eHostCoherent);

    return m_vulkanDevice->GetVkDevice().mapMemory(m_vkDeviceMemory, offset, size);
}

void VulkanDeviceMemory::UnMapMemory()
{
    ZoneScopedN("VulkanDeviceMemory::UnMapMemory");
    m_vulkanDevice->GetVkDevice().unmapMemory(m_vkDeviceMemory);
}

void VulkanDeviceMemory::Bind(VulkanBuffer* buf)
{
    ZoneScopedN("VulkanDeviceMemory::Bind");
    m_vulkanDevice->GetVkDevice().bindBufferMemory(*buf->GetPVkBuf(), m_vkDeviceMemory, 0);
}

void VulkanDeviceMemory::Bind(VulkanImageResource* img)
{
    ZoneScopedN("VulkanDeviceMemory::Bind");
    m_vulkanDevice->GetVkDevice().bindImageMemory(img->GetVkImage(), m_vkDeviceMemory, 0);
}

uint32_t VulkanDeviceMemory::findMemoryType()
{
    auto& memoryProps = m_vulkanDevice->GetVulkanPhysicalDevice()->GetVkPhysicalDeviceMemoryProps();

    for (int i = 0; i < memoryProps.memoryTypeCount; i++)
    {
        if (
            (m_vkMemRequirements.memoryTypeBits & (1 << i))
            &&
            ((memoryProps.memoryTypes[i].propertyFlags & m_vkMemProps) == m_vkMemProps)
        )
        {
            return i;
        }
    }

    throw std::runtime_error("faild to find suitable memory type");
    return 0;
}

VulkanBuffer::VulkanBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode)
    : m_vulkanDevice(device)
    , m_vkSize(size)
    , m_vkBufUsage(usage)
    , m_vkSharingMode(sharingMode)
{
    ZoneScopedN("VulkanBuffer::VulkanBuffer");
    auto bufferInfo = vk::BufferCreateInfo()
                    .setSize(m_vkSize)
                    .setUsage(m_vkBufUsage)
                    .setSharingMode(m_vkSharingMode);
    m_vkBuf = m_vulkanDevice->GetVkDevice().createBuffer(bufferInfo);

    vk::MemoryRequirements requirement = m_vulkanDevice->GetVkDevice().getBufferMemoryRequirements(m_vkBuf);

    m_pVulkanDeviceMemory.reset(new VulkanDeviceMemory(device, requirement, properties));
    m_pVulkanDeviceMemory->Bind(this);
}


VulkanBuffer::~VulkanBuffer()
{
    ZoneScopedN("VulkanBuffer::~VulkanBuffer");
    destroy();
}

void VulkanBuffer::destroy()
{
    Unmapping();
    if (m_vkBuf)
    {
        m_vulkanDevice->GetVkDevice().destroyBuffer(m_vkBuf);
    }
    m_pVulkanDeviceMemory.reset();
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


void VulkanBuffer::FillingBufferOneTime(void* data, std::size_t offset, std::size_t size)
{
    ZoneScopedN("VulkanBuffer::FillingBufferOneTime");
    assert(offset + size <= m_vkSize);

    void* dstData = m_pVulkanDeviceMemory->MapMemory(offset, size);
    memcpy(dstData, data, size);
    //m_vulkanDevice->GetVkDevice().flushMappedMemoryRanges(...);
    m_pVulkanDeviceMemory->UnMapMemory();
}

void VulkanBuffer::FillingMappingBuffer(void* data, std::size_t offset, std::size_t size)
{
    ZoneScopedN("VulkanBuffer::FillingMappingBuffer");
    assert(offset + size <= m_vkSize);
    if (m_mappedPointer == nullptr)
    {
        m_mappedPointer = m_pVulkanDeviceMemory->MapMemory(offset, size);
    }
    memcpy(m_mappedPointer, data, size);
}

void VulkanBuffer::Unmapping()
{
    ZoneScopedN("VulkanBuffer::Unmapping");
    if (m_mappedPointer)
    {
        m_pVulkanDeviceMemory->UnMapMemory();
    }
    m_mappedPointer = nullptr;
}


VulkanGPUBuffer::VulkanGPUBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode)
    : VulkanBuffer(device, size, usage, props, sharingMode)
{
    ZoneScopedN("VulkanGPUBuffer::VulkanGPUBuffer");
    m_pVulkanCPUBuffer.reset(new VulkanBuffer(device, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, sharingMode));
}

VulkanGPUBuffer::~VulkanGPUBuffer() 
{
    ZoneScopedN("VulkanGPUBuffer::~VulkanGPUBuffer");
    m_pVulkanCPUBuffer.reset();
}



void VulkanGPUBuffer::FillingBufferOneTime(void* data, std::size_t size, std::size_t offset)
{
    ZoneScopedN("VulkanGPUBuffer::FillingBufferOneTime");
    m_pVulkanCPUBuffer->FillingBufferOneTime(data, offset, size);
}

void VulkanGPUBuffer::CopyDataToGPU(vk::CommandBuffer cmd, vk::Queue queue, std::size_t size, std::size_t dstOffset, std::size_t srcOffset)
{
    ZoneScopedN("VulkanGPUBuffer::CopyDataToGPU");
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