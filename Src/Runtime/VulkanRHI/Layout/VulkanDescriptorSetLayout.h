#pragma once
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include "Runtime/VulkanRHI/VulkanRHI.h"


RHI_NAMESPACE_BEGIN

class VulkanShaderSet;
class VulkanDevice;
class VulkanDescriptorSetLayout
{
public:
    static constexpr uint32_t DESCRIPTOR_MVPUBO_BINDING_ID = 0;
    static constexpr uint32_t DESCRIPTOR_SAMPLER1_BINDING_ID = 1;
    static constexpr uint32_t DESCRIPTOR_SAMPLER2_BINDING_ID = 2;
    static constexpr uint32_t DESCRIPTOR_SAMPLER3_BINDING_ID = 3;
    static constexpr uint32_t DESCRIPTOR_SAMPLER4_BINDING_ID = 4;
    static constexpr uint32_t DESCRIPTOR_SAMPLER5_BINDING_ID = 5;
protected:
    VulkanDevice* m_vulkanDevice = nullptr;

    std::string name = "empty";

    vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
public:
    explicit VulkanDescriptorSetLayout(VulkanDevice* device);
    virtual ~VulkanDescriptorSetLayout();
    void AddBinding(uint32_t id, vk::DescriptorSetLayoutBinding binding);
    virtual void Finish();
    vk::DescriptorSetLayout& GetVkDescriptorSetLayout();
    virtual const char* GetName() { return "empty"; }
};

struct VulkanDescriptorSetLayoutPresets
{
    static std::shared_ptr<VulkanDescriptorSetLayout> OnlyMVPUBO;
    static std::shared_ptr<VulkanDescriptorSetLayout> Custom5Sampler;
    static void Init(VulkanDevice* device);
    static void UnInit();
};

class VulkanMVPUBODescriptorSetLayout final : public VulkanDescriptorSetLayout
{
public:
    explicit VulkanMVPUBODescriptorSetLayout(VulkanDevice* device) : VulkanDescriptorSetLayout(device) { }
    static const vk::DescriptorSetLayoutBinding& GetVkBinding();
    void Finish() override;
    const char* GetName() override { return "MVPUBO"; }
};

class VulkanCustom5SamplerDescriptorSetLayout : public VulkanDescriptorSetLayout
{
public:
    explicit VulkanCustom5SamplerDescriptorSetLayout(VulkanDevice* device) : VulkanDescriptorSetLayout(device) { }
    static const std::vector<vk::DescriptorSetLayoutBinding>& GetVkBinding();
    void Finish() override;
    const char* GetName() override { return "Custom5Sampler"; }
};
RHI_NAMESPACE_END