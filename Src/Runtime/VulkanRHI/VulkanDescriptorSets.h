#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

class VulkanImageSampler;
class VulkanDescriptorSets
{
public:
private:
    VulkanDevice* m_vulkanDevice = nullptr;
    VulkanDescriptorSetLayout* m_vulkanDescLayout = nullptr;

    vk::DescriptorPool m_vkDescPool;
    std::vector<vk::DescriptorSet> m_vkDescSets;
    std::vector<VulkanBuffer*> m_pVulkanUniformBuffers;
    std::vector<VulkanImageSampler*> m_pVulkanImageSamplers;
    std::vector<uint32_t> m_binding;

public:
    explicit VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        const std::vector<VulkanBuffer*>& uniformBuffers,
        const std::vector<uint32_t>& binding
    );

    explicit VulkanDescriptorSets(
        VulkanDevice* device,
        vk::DescriptorPool descPool,
        VulkanDescriptorSetLayout* layout,
        std::vector<VulkanImageSampler*> imageSamplers,
        const std::vector<uint32_t>& binding
    );

    ~VulkanDescriptorSets();

    VulkanDescriptorSetLayout* GetPDescriptorSetLayout() { return m_vulkanDescLayout; }
    std::vector<vk::DescriptorSet> GetVkDescriptorSets(const std::vector<int>& setIdx = {});
    inline vk::DescriptorSet& GetVkDescriptorSet(int idx) { return m_vkDescSets[idx]; }
    inline uint32_t GetBinding(int idx) { return m_binding[idx]; }

    void FillToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout, int selfSetIndex = 0);
    void BindGraphicPipelinePoint(vk::CommandBuffer cmd, vk::PipelineLayout layout, const std::vector<int>& setIdx = {}, int firstIdx = 0);
};

struct VulkanDescriptorSetInfo
{
    std::shared_ptr<VulkanDescriptorSets> descriptorSets;
    int index;
};
RHI_NAMESPACE_END