#pragma once
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <vcruntime.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanBuffer;
class VulkanImage;
class VulkanDeviceMemory
{
private:
    VulkanDevice* m_vulkanDevice;
    vk::DeviceMemory m_vkDeviceMemory;
    vk::MemoryRequirements m_vkMemRequirements;
    vk::MemoryPropertyFlags m_vkMemProps;
public:
    explicit VulkanDeviceMemory(VulkanDevice* device, vk::MemoryRequirements requirements, vk::MemoryPropertyFlags props);
    ~VulkanDeviceMemory();
    void* MapMemory(std::size_t offset, std::size_t size);
    void UnMapMemory();
    void Bind(VulkanBuffer* buf);
    void Bind(VulkanImage* img);
private:
    uint32_t findMemoryType();
};

class VulkanBuffer
{
public:
protected:
    vk::Buffer m_vkBuf;
    std::unique_ptr<VulkanDeviceMemory> m_pVulkanDeviceMemory;
    VulkanDevice* m_vulkanDevice;
    vk::DeviceSize m_vkSize;
    vk::BufferUsageFlags m_vkBufUsage;
    vk::SharingMode m_vkSharingMode;
    void* m_mappedPointer = nullptr;
public:
    explicit VulkanBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode);
    VulkanBuffer() = delete;
    virtual ~VulkanBuffer();

    inline vk::Buffer* GetPVkBuf() { return &m_vkBuf; }
    void FillingBufferOneTime(void* data, std::size_t offset, std::size_t size);
    void FillingMappingBuffer(void* data, std::size_t offset, std::size_t size);
    void Unmapping();
protected:
    void destroy();
private:
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};


class VulkanGPUBuffer : public VulkanBuffer
{
protected:
    std::unique_ptr<VulkanBuffer> m_pVulkanCPUBuffer;
public:
    explicit VulkanGPUBuffer(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode); 
    ~VulkanGPUBuffer() override;
    void FillingBufferOneTime(void* data, std::size_t size, std::size_t offset = 0);
    void CopyDataToGPU(vk::CommandBuffer cmd, vk::Queue queue, std::size_t size, std::size_t dstOffset = 0, std::size_t srcOffset = 0);
};

template<typename T>
class tVulkanGPUBuffer : public VulkanGPUBuffer
{
private:
    explicit tVulkanGPUBuffer<T>(VulkanDevice*device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode = vk::SharingMode::eExclusive)
        : VulkanGPUBuffer(device, size, usage, props, sharingMode)
    {
    }
public:
    ~tVulkanGPUBuffer<T>() override
    {
    }

    void FillingBufferOneTime(const std::vector<T>& data, std::size_t offset = 0)
    {
        m_pVulkanCPUBuffer->FillingBufferOneTime((void*)data.data(), offset, data.size() * sizeof(T));
    }

    static std::unique_ptr<tVulkanGPUBuffer<T>> Create(VulkanDevice* device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode = vk::SharingMode::eExclusive)
    {
        if (!(usage & vk::BufferUsageFlagBits::eTransferDst))
        {
            usage |= vk::BufferUsageFlagBits::eTransferDst;
        }

        if (!(props & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            props |= vk::MemoryPropertyFlagBits::eDeviceLocal;
        }

        std::unique_ptr<tVulkanGPUBuffer<T>> buf(new tVulkanGPUBuffer<T>(device, size, usage, props, sharingMode));
        return buf;
    }

    static std::unique_ptr<tVulkanGPUBuffer<T>> Create(VulkanDevice* device, const std::vector<T>& data, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props, vk::SharingMode sharingMode = vk::SharingMode::eExclusive)
    {
        std::unique_ptr<tVulkanGPUBuffer<T>> buf = tVulkanGPUBuffer<T>::Create(device, data.size() * sizeof(T), usage, props, sharingMode);
        buf->FillingBufferOneTime(data);
        return buf;
    }
};

using VulkanVertexBuffer = tVulkanGPUBuffer<Vertex>;
using VulkanVertexIndexBuffer = tVulkanGPUBuffer<uint16_t>;

RHI_NAMESPACE_END