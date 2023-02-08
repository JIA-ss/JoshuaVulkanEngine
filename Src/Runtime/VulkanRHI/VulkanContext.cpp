#include "VulkanContext.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Util/fileutil.h"
#include "Util/textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <assert.h>
#include <stdio.h>
RHI_NAMESPACE_USING

std::unique_ptr<VulkanContext> VulkanContext::s_instance = nullptr;

VulkanContext& VulkanContext::CreateInstance()
{
    assert(!s_instance);
    s_instance.reset(new VulkanContext());
    return *s_instance;
}
void VulkanContext::DestroyInstance()
{
    assert(s_instance);
    s_instance.reset();
}

VulkanContext& VulkanContext::GetInstance()
{
    assert(s_instance);
    return *s_instance;
}

void VulkanContext::Init(
    const VulkanInstance::Config& instanceConfig,
    const VulkanPhysicalDevice::Config& physicalConfig)
{
    m_pInstance.reset(new VulkanInstance(instanceConfig));
    m_pPhysicalDevice.reset(new VulkanPhysicalDevice(physicalConfig, m_pInstance.get()));
    m_pDevice.reset(new VulkanDevice(m_pPhysicalDevice.get()));
    m_pShaderSet.reset(new VulkanShaderSet(m_pDevice.get()));

    createVulkanDescriptorSetLayout();
    createVulkanDescriptorSet();

    m_pShaderSet->AddShader(util::file::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    m_pShaderSet->AddShader(util::file::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    
    m_pRenderPipeline.reset(new VulkanRenderPipeline(m_pDevice.get(), m_pShaderSet.get(), m_pDescSetLayout.get(), nullptr));
    
    m_pDevice->CreateSwapchainFramebuffer(m_pRenderPipeline.get());
}

void VulkanContext::createVulkanDescriptorSetLayout()
{
    m_pDescSetLayout.reset(new VulkanDescriptorSetLayout(m_pDevice.get()));
    std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
    bindings[0]
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            ;
    bindings[1]
            .setBinding(1)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ;
    m_pDescSetLayout->AddBindings(bindings);
    m_pDescSetLayout->Finish();
}

void VulkanContext::createVulkanDescriptorSet()
{
    std::vector<std::unique_ptr<VulkanBuffer>> uniformBuffers(MAX_FRAMES_IN_FLIGHT);
    std::vector<std::unique_ptr<VulkanImageSampler>> images(MAX_FRAMES_IN_FLIGHT);
    auto imageRawData = util::texture::RawData::Load(util::file::getResourcePath() / "Texture/texture.jpg", util::texture::RawData::Format::eRgbAlpha);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uniformBuffers[i].reset(
            new VulkanBuffer(
                    m_pDevice.get(), sizeof(UniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                )
        );
        VulkanImageSampler::Config imageSamplerConfig;
        VulkanImageResource::Config imageResourceConfig;
        imageResourceConfig.extent = vk::Extent3D{(uint32_t)imageRawData->GetWidth(), (uint32_t)imageRawData->GetHeight(), 1};
        images[i].reset(
            new VulkanImageSampler(
                m_pDevice.get(),
                imageRawData,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                imageSamplerConfig,
                imageResourceConfig
                )
        );
    }

    m_pDescSet.reset(new VulkanDescriptorSets(m_pDevice.get(), m_pDescSetLayout.get(), std::move(uniformBuffers), std::move(images)));
}

void VulkanContext::Destroy()
{
    m_pRenderPipeline.reset();
    m_pDescSetLayout.reset();
    m_pDescSet.reset();
    m_pShaderSet.reset();
    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}