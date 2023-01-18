#include "VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Util/fileutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

VulkanShaderSet::~VulkanShaderSet()
{
    for (auto & [flag,shader] : m_shaderStageCreateInfos)
    {
        m_vulkanDevice->GetVkDevice().destroyShaderModule(shader.module);
    }

    m_shaderStageCreateInfos.clear();
}


bool VulkanShaderSet::AddShader(const boost::filesystem::path &spvFile, vk::ShaderStageFlagBits type, const char *entryPoint)
{
    auto it = m_shaderStageCreateInfos.find(type);
    if (it != m_shaderStageCreateInfos.end())
    {
        return false;
    }

    auto module = createShaderModule(spvFile);
    if (!module)
    {
        return false;
    }

    m_shaderStageCreateInfos[type].setStage(type)
                .setModule(module)
                .setPName(entryPoint);
    return true;
}


vk::ShaderModule VulkanShaderSet::createShaderModule(const boost::filesystem::path& spvFile)
{
    vk::ShaderModule shader;
    std::vector<char> code;
    if (!util::file::readFile(spvFile, code) || code.empty())
    {
        return shader;
    }

    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(code.size())
                .setPCode((const uint32_t*)(code.data()));
    shader = m_vulkanDevice->GetVkDevice().createShaderModule(createInfo);
    return shader;
}