#pragma once

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <boost/filesystem/path.hpp>
#include <map>
RHI_NAMESPACE_BEGIN

class VulkanDevice;
class VulkanShaderSet
{
public:
private:
    VulkanDevice* m_vulkanDevice = nullptr;

    std::map<vk::ShaderStageFlagBits, vk::PipelineShaderStageCreateInfo> m_shaderStageCreateInfos;
    std::vector<vk::DescriptorSetLayoutBinding> m_descriptorSetLayoutBindings;
public:
    explicit VulkanShaderSet(VulkanDevice* device) : m_vulkanDevice(device)
    {
    }
    ~VulkanShaderSet();

    bool AddShader(const boost::filesystem::path& spvFile, vk::ShaderStageFlagBits type, const char *entryPoint = "main");
    void AppendVertexAttributeDescription();

    const std::vector<vk::DescriptorSetLayoutBinding>& GetDescriptorSetLayoutBindings() { return m_descriptorSetLayoutBindings; }
private:
    vk::ShaderModule createShaderModule(const boost::filesystem::path& spvFile);
};

RHI_NAMESPACE_END