#pragma once

#include "Runtime/VulkanRHI/VulkanRHI.h"

#include <vulkan/vulkan.hpp>
#include <boost/filesystem/path.hpp>
RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanPipelineCache
{
public:
private:
    vk::PhysicalDeviceProperties m_vkPhysicalDeviceProps;
    vk::PipelineCache m_vkPipelineCache;

    VulkanDevice* m_pVulkanDevice = nullptr;
    boost::filesystem::path m_cacheFilePath;
public:
    explicit VulkanPipelineCache(VulkanDevice* device, vk::PhysicalDeviceProperties props, const boost::filesystem::path& path);
    ~VulkanPipelineCache();

    bool LoadGraphicsPipelineCache(const boost::filesystem::path& file);
    bool SaveGraphicsPipelineCache(const boost::filesystem::path& file);
private:
    bool isCacheValid(const boost::filesystem::path& file);
    bool isCacheValid(const std::vector<char>& buffer);
};

RHI_NAMESPACE_END