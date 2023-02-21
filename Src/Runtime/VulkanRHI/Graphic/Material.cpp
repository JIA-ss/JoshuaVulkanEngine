#include "Material.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_handles.hpp"

RHI_NAMESPACE_USING

Material::Material(VulkanDevice* device, const std::vector<std::shared_ptr<VulkanDescriptorSets>>& descriptors)
    : m_pVulkanDevice(device)
    , m_pVulkanDescriptorSets(descriptors)
{

}


void Material::bind(vk::CommandBuffer& cmd, VulkanPipelineLayout* pipelineLayout, std::vector<vk::DescriptorSet>& tobinding)
{
    for (auto& desc : m_pVulkanDescriptorSets)
    {
        desc->FillToBindedDescriptorSetsVector(tobinding, pipelineLayout);
    }
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
}


std::array<std::unique_ptr<RHI::VulkanBuffer>, MAX_FRAMES_IN_FLIGHT>&& Material::initUniformBuffers()
{
    // descriptor set
    std::array<std::unique_ptr<RHI::VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uniformBuffers[i].reset(
            new RHI::VulkanBuffer(
                    m_pVulkanDevice, sizeof(UniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                )
        );
    }
    return std::move(uniformBuffers);
}

std::vector<std::unique_ptr<RHI::VulkanImageSampler>>&& Material::initImageSamplers()
{
    std::vector<std::unique_ptr<RHI::VulkanImageSampler>> images;
    for (auto& textureData : m_materialData.textureDatas)
    {
        auto imageRawData = textureData.rawData;
            RHI::VulkanImageSampler::Config imageSamplerConfig;
            RHI::VulkanImageResource::Config imageResourceConfig;
            imageResourceConfig.extent = vk::Extent3D{(uint32_t)imageRawData->GetWidth(), (uint32_t)imageRawData->GetHeight(), 1};
            images.emplace_back(
                new RHI::VulkanImageSampler(
                    m_pVulkanDevice,
                    imageRawData,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    imageSamplerConfig,
                    imageResourceConfig
                    )
            );
    }

    return std::move(images);
}