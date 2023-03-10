#pragma once
#include <map>
#include <stdint.h>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_BEGIN

class VulkanDescriptorSets;
class VulkanDescriptorSetLayout;
class VulkanDevice;
class VulkanPipelineLayout
{
public:
private:
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_vulkanDescSetLayouts;
    VulkanDevice* m_vulkanDevice = nullptr;

    vk::PipelineLayout m_vkPipelineLayout;
    std::map<int, vk::PushConstantRange> m_vkPushConstRanges; // <offset, range>
    std::vector<const char*> m_DescriptorSetLayoutNames;
public:
    explicit VulkanPipelineLayout(VulkanDevice* device, std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> descLayouts, const std::map<int, vk::PushConstantRange>& pushConstant = {});
    ~VulkanPipelineLayout();

    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> GetVulkanDescriptorSetLayouts() { return m_vulkanDescSetLayouts; }
    inline vk::PipelineLayout& GetVkPieplineLayout() { return m_vkPipelineLayout; }

    int GetDescriptorSetId(const char* descLayoutName);
    int GetDescriptorSetId(VulkanDescriptorSetLayout* layout);
    int GetDescriptorSetId(VulkanDescriptorSets* desc);

    VulkanDescriptorSetLayout* GetPVulkanDescriptorSet(int SETID) { return m_vulkanDescSetLayouts[SETID].get(); }

    void PushConstant(vk::CommandBuffer cmd, uint32_t offset, uint32_t size, void* data, vk::ShaderStageFlags stage);
    template<typename T>
    void PushConstantT(vk::CommandBuffer cmd, uint32_t offset, const T& data, vk::ShaderStageFlags stage)
    {
        PushConstant(cmd, offset, sizeof(T), (void*)&data, stage);
    }
private:
    bool checkPushContantRangeValid(std::vector<vk::PushConstantRange>& validRanges);
};
RHI_NAMESPACE_END