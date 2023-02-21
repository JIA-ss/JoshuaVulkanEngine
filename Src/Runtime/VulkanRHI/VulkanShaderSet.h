#pragma once

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan.hpp"
#include <boost/filesystem/path.hpp>
#include <map>
#include <stdint.h>
RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanShaderSet
{
public:
private:
    VulkanDevice* m_vulkanDevice = nullptr;
    std::map<vk::ShaderStageFlagBits, vk::PipelineShaderStageCreateInfo> m_shaderStageCreateInfos;

public:
    explicit VulkanShaderSet(VulkanDevice* device)
        : m_vulkanDevice(device)
    {
    }
    ~VulkanShaderSet();

    bool AddShader(const boost::filesystem::path& spvFile, vk::ShaderStageFlagBits type, const char *entryPoint = "main");

    std::vector<vk::PipelineShaderStageCreateInfo> GetShaderCreateInfos();
private:
    vk::ShaderModule createShaderModule(const boost::filesystem::path& spvFile);
};

RHI_NAMESPACE_END