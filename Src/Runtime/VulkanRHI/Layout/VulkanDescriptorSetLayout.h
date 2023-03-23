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
    // SET 0 (UniformBuffer)
    static constexpr uint32_t DESCRIPTOR_CAMVPUBO_BINDING_ID = 0;
    static constexpr uint32_t DESCRIPTOR_LIGHTUBO_BINDING_ID = 1;
    static constexpr uint32_t DESCRIPTOR_MODELUBO_BINDING_ID = 2;
    static constexpr uint32_t DESCRIPTOR_CUSTOMUBO_BINDING_ID = 3;
    // SET 1 (Custom Sampler)
    static constexpr uint32_t DESCRIPTOR_SAMPLER1_BINDING_ID = 1;
    static constexpr uint32_t DESCRIPTOR_SAMPLER2_BINDING_ID = 2;
    static constexpr uint32_t DESCRIPTOR_SAMPLER3_BINDING_ID = 3;
    static constexpr uint32_t DESCRIPTOR_SAMPLER4_BINDING_ID = 4;
    static constexpr uint32_t DESCRIPTOR_SAMPLER5_BINDING_ID = 5;
    // SET 2 (SHADOWMAP)
    static constexpr uint32_t DESCRIPTOR_SHADOWMAP1_BINDING_ID = 1;
    static constexpr uint32_t DESCRIPTOR_SHADOWMAP2_BINDING_ID = 2;
    static constexpr uint32_t DESCRIPTOR_SHADOWMAP3_BINDING_ID = 3;
    static constexpr uint32_t DESCRIPTOR_SHADOWMAP4_BINDING_ID = 4;
    static constexpr uint32_t DESCRIPTOR_SHADOWMAP5_BINDING_ID = 5;
protected:
    VulkanDevice* m_vulkanDevice = nullptr;

    std::string name = "empty";

    vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
public:
    explicit VulkanDescriptorSetLayout(VulkanDevice* device);
    explicit VulkanDescriptorSetLayout(VulkanDevice* device, vk::DescriptorSetLayout vkDescriptorSetLayout);
    virtual ~VulkanDescriptorSetLayout();
    void AddBinding(uint32_t id, vk::DescriptorSetLayoutBinding binding);
    virtual void Finish();
    vk::DescriptorSetLayout& GetVkDescriptorSetLayout();
    virtual const char* GetName() { return "empty"; }
};

struct VulkanDescriptorSetLayoutPresets
{
    std::shared_ptr<VulkanDescriptorSetLayout> UBO;
    std::shared_ptr<VulkanDescriptorSetLayout> CUSTOM5SAMPLER;
    std::shared_ptr<VulkanDescriptorSetLayout> SHADOWMAP;

    std::shared_ptr<VulkanDescriptorSetLayout> CreateCustomUBO(VulkanDevice* device, vk::ShaderStageFlags stage);
    void Init(VulkanDevice* device);
    void UnInit();
};

class VulkanUBODescriptorSetLayout final : public VulkanDescriptorSetLayout
{
public:
    explicit VulkanUBODescriptorSetLayout(VulkanDevice* device) : VulkanDescriptorSetLayout(device) { }
    static const std::vector<vk::DescriptorSetLayoutBinding>& GetVkBinding();
    void Finish() override;
    const char* GetName() override { return "MVPUBO"; }
};

class VulkanCustom5SamplerDescriptorSetLayout : public VulkanDescriptorSetLayout
{
public:
    explicit VulkanCustom5SamplerDescriptorSetLayout(VulkanDevice* device) : VulkanDescriptorSetLayout(device) { }
    static const std::vector<vk::DescriptorSetLayoutBinding>& GetVkBinding();
    void Finish() override;
    const char* GetName() override { return "CUSTOM5SAMPLER"; }
};

class VulkanShadowMapDescriptorSetLayout : public VulkanDescriptorSetLayout
{
public:
    explicit VulkanShadowMapDescriptorSetLayout(VulkanDevice* device) : VulkanDescriptorSetLayout(device) { }
    static const std::vector<vk::DescriptorSetLayoutBinding>& GetVkBinding();
    void Finish() override;
    const char* GetName() override { return "SHADOWMAP"; }
};
RHI_NAMESPACE_END