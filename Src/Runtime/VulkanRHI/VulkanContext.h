#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "VulkanRHI.h"
#include "vulkan/vulkan_enums.hpp"
#include <memory>
RHI_NAMESPACE_BEGIN

class VulkanContext final
{
public:
private:
    static std::unique_ptr<VulkanContext> s_instance;

    std::unique_ptr<VulkanInstance> m_pInstance;
    std::unique_ptr<VulkanPhysicalDevice> m_pPhysicalDevice;
    std::unique_ptr<VulkanDevice> m_pDevice;
    std::shared_ptr<VulkanDescriptorSetLayout> m_pDescSetLayout;
    std::shared_ptr<VulkanDescriptorSets> m_pUniformSets;
    std::shared_ptr<VulkanDescriptorSets> m_pSamplerSets;
    std::shared_ptr<VulkanShaderSet> m_pShaderSet;
    std::shared_ptr<VulkanRenderPipeline> m_pRenderPipeline;

    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
    std::vector<std::unique_ptr<VulkanImageSampler>> m_images;

    std::unique_ptr<VulkanDescriptorPool> m_pDescPool;
public:
    static VulkanContext& CreateInstance();
    static void DestroyInstance();
    static VulkanContext& GetInstance();

    void operator=(const VulkanContext&) = delete;
    VulkanContext(const VulkanContext&) = delete;

public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    inline VulkanBuffer* GetPUniformBuffer(int idx) { return m_uniformBuffers[idx].get(); }
    inline VulkanDescriptorSets* GetPUniformVulkanDescriptorSets() { return m_pUniformSets.get(); }
    inline VulkanDescriptorSets* GetPSamplerVulkanDescriptorSets() { return m_pSamplerSets.get(); }
    inline VulkanInstance& GetVulkanInstance() { assert(m_pInstance); return *m_pInstance; }
    inline VulkanPhysicalDevice& GetVulkanPhysicalDevice() { assert(m_pPhysicalDevice); return *m_pPhysicalDevice; }
    inline VulkanDevice& GetVulkanDevice() { assert(m_pDevice); return *m_pDevice; }
    inline VulkanRenderPipeline& GetVulkanRenderPipeline() { assert(m_pRenderPipeline); return *m_pRenderPipeline;}
    void Init(const VulkanInstance::Config& instanceConfig,
                const VulkanPhysicalDevice::Config& physicalConfig);
    void Destroy();
private:
    void createVulkanDescriptorSetLayout();
    void createVulkanDescriptorSet();
};

RHI_NAMESPACE_END