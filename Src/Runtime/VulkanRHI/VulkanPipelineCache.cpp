#include "VulkanPipelineCache.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_structs.hpp"
#include <iomanip>
#include <iostream>


RHI_NAMESPACE_USING

void printUUID(uint8_t *pipelineCacheUUID)
{
    for (size_t j = 0; j < VK_UUID_SIZE; ++j) {
        std::cout << std::setw(2) << (uint32_t)pipelineCacheUUID[j];
        if (j == 3 || j == 5 || j == 7 || j == 9) {
            std::cout << '-';
        }
    }
}

VulkanPipelineCache::VulkanPipelineCache(VulkanDevice* device, vk::PhysicalDeviceProperties props, const boost::filesystem::path& path)
    : m_pVulkanDevice(device),
        m_vkPhysicalDeviceProps(props),
        m_cacheFilePath(path)
{
    LoadGraphicsPipelineCache(path);
}

VulkanPipelineCache::~VulkanPipelineCache()
{
    m_pVulkanDevice->GetVkDevice().destroyPipelineCache(m_vkPipelineCache);
    m_vkPipelineCache = nullptr;
}

bool VulkanPipelineCache::LoadGraphicsPipelineCache(const boost::filesystem::path& File)
{
    std::vector<char> buf;
    bool cacheFileValid = true;
    if (!Util::File::readFile(File, buf))
    {
        cacheFileValid = false;
    }

    if (!isCacheValid(buf))
    {
        cacheFileValid = false;
    }

    vk::PipelineCacheCreateInfo createInfo;
    createInfo.setPInitialData(cacheFileValid ? buf.data() : nullptr)
                .setInitialDataSize(cacheFileValid ? buf.size() : 0);
    m_vkPipelineCache = m_pVulkanDevice->GetVkDevice().createPipelineCache(createInfo);
    return cacheFileValid;
}

bool VulkanPipelineCache::SaveGraphicsPipelineCache(const boost::filesystem::path& File)
{
    if (File.empty())
    {
        return false;
    }
    std::vector<unsigned char> res = m_pVulkanDevice->GetVkDevice().getPipelineCacheData(m_vkPipelineCache);
    if (res.empty())
    {
        return false;
    }
    return Util::File::writeFile(File, res.data(), res.size());
}

bool VulkanPipelineCache::isCacheValid(const boost::filesystem::path& File)
{

    std::vector<char> buffer;
    if (!Util::File::readFile(File, buffer))
    {
        return false;
    }

    return isCacheValid(buffer);
}

bool VulkanPipelineCache::isCacheValid(const std::vector<char>& buf)
{
    if (buf.size() > 32)
    {
        char* buffer = (char*)buf.data();
        uint32_t header = 0;
        uint32_t version = 0;
        uint32_t vendor = 0;
        uint32_t deviceID = 0;
        uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

        memcpy(&header, (uint8_t *)buffer + 0, 4);
        memcpy(&version, (uint8_t *)buffer + 4, 4);
        memcpy(&vendor, (uint8_t *)buffer + 8, 4);
        memcpy(&deviceID, (uint8_t *)buffer + 12, 4);
        memcpy(pipelineCacheUUID, (uint8_t *)buffer + 16, VK_UUID_SIZE);

        if (header <= 0) {
            std::cerr << "bad pipeline cache data header" << std::endl;
            return false;
        }

        if (version != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) {
            std::cerr << "unsupported cache header version " << std::endl;
            std::cerr << "cache contains: 0x" << std::hex << version << std::endl;
        }

        if (vendor != m_vkPhysicalDeviceProps.vendorID) {
            std::cerr << "vendor id mismatch " << std::endl;
            std::cerr << "cache contains: 0x" << std::hex << vendor << std::endl;
            std::cerr << "driver expects: 0x" << m_vkPhysicalDeviceProps.vendorID << std::endl;
            return false;
        }

        if (deviceID != m_vkPhysicalDeviceProps.deviceID) {
            std::cerr << "device id mismatch in" << std::endl;
            std::cerr << "cache contains: 0x" << std::hex << deviceID << std::endl;
            std::cerr << "driver expects: 0x" << m_vkPhysicalDeviceProps.deviceID << std::endl;
            return false;
        }

        if (memcmp(pipelineCacheUUID, m_vkPhysicalDeviceProps.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0) {
            std::cerr << "uuid mismatch " << std::endl;
            std::cerr << "cache contains: " << std::endl;


            printUUID(pipelineCacheUUID);

            std::cerr << "driver expects:" << std::endl;
            printUUID(m_vkPhysicalDeviceProps.pipelineCacheUUID);
        };

        return true;
    }
    return false;
}