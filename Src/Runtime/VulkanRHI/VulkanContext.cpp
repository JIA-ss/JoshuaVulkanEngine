#include "VulkanContext.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Util/Fileutil.h"
#include "Util/Textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <assert.h>
#include <stdio.h>
#include <vector>
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

    m_pShaderSet->AddShader(Util::File::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    m_pShaderSet->AddShader(Util::File::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    m_pRenderPipeline = VulkanRenderPipelineBuilder(m_pDevice.get(), nullptr)
                        .SetshaderSet(m_pShaderSet)
                        .AddDescriptorLayout(m_pDescSetLayout)
                        .buildShared();
    m_pDevice->CreateSwapchainFramebuffer(m_pRenderPipeline->GetVulkanRenderPass());
}



void VulkanContext::createVulkanDescriptorSetLayout()
{
    m_pDescSetLayout.reset(new VulkanDescriptorSetLayout(m_pDevice.get()));

    m_pDescSetLayout->AddBinding(0, vk::DescriptorSetLayoutBinding()
                        .setBinding(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                    );
    m_pDescSetLayout->AddBinding(1, vk::DescriptorSetLayoutBinding()
                        .setBinding(1)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                    );
    m_pDescSetLayout->Finish();
}

void VulkanContext::createVulkanDescriptorSet()
{
    m_images.resize(MAX_FRAMES_IN_FLIGHT);
    std::vector<VulkanBuffer*> pBuffers(MAX_FRAMES_IN_FLIGHT);
    std::vector<VulkanImageSampler *> pImageSamplers(MAX_FRAMES_IN_FLIGHT);

    auto imageRawData = Util::Texture::RawData::Load(Util::File::getResourcePath() / "Texture/viking_room.png", Util::Texture::RawData::Format::eRgbAlpha);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_uniformBuffers[i].reset(
            new VulkanBuffer(
                    m_pDevice.get(), sizeof(CameraUniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                )
        );
        pBuffers[i] = m_uniformBuffers[i].get();
        VulkanImageSampler::Config imageSamplerConfig;
        VulkanImageResource::Config imageResourceConfig;
        imageResourceConfig.extent = vk::Extent3D{(uint32_t)imageRawData->GetWidth(), (uint32_t)imageRawData->GetHeight(), 1};
        m_images[i].reset(
            new VulkanImageSampler(
                m_pDevice.get(),
                imageRawData,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                imageSamplerConfig,
                imageResourceConfig
                )
        );
        pImageSamplers[i] = m_images[i].get();
    }
    std::vector<vk::DescriptorPoolSize> poolSizes
    {
        vk::DescriptorPoolSize
        {
        vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize
        {
        vk::DescriptorType::eCombinedImageSampler, (uint32_t)m_images.size()
        },
    };
    m_pDescPool.reset(new VulkanDescriptorPool(m_pDevice.get(), poolSizes, poolSizes.size()));

    std::vector<uint32_t> uniformBinding(MAX_FRAMES_IN_FLIGHT, VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID);
    std::vector<uint32_t> range(MAX_FRAMES_IN_FLIGHT, sizeof(CameraUniformBufferObject));
    m_pUniformSets = m_pDescPool->AllocUniformDescriptorSet(m_pDescSetLayout.get(), pBuffers, uniformBinding, range, MAX_FRAMES_IN_FLIGHT);

    std::vector<uint32_t> samplerBinding(1, VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER1_BINDING_ID);
    for (int i = 1; i < m_images.size(); i++)
    {
        samplerBinding.emplace_back(VulkanDescriptorSetLayout::DESCRIPTOR_SAMPLER1_BINDING_ID + i);
    }
    m_pSamplerSets = m_pDescPool->AllocSamplerDescriptorSet(m_pDescSetLayout.get(), pImageSamplers, samplerBinding);
}

void VulkanContext::Destroy()
{
    m_pRenderPipeline.reset();
    m_pDescSetLayout.reset();
    m_pDescPool.reset();
    m_pShaderSet.reset();
    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}