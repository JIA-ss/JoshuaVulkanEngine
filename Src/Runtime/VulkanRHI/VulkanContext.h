#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "VulkanRHI.h"
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
    std::unique_ptr<VulkanDescriptorSetLayout> m_pDescSetLayout;
    std::unique_ptr<VulkanDescriptorSets> m_pDescSet;
    std::unique_ptr<VulkanShaderSet> m_pShaderSet;
    std::unique_ptr<VulkanRenderPipeline> m_pRenderPipeline;
public:
    static VulkanContext& CreateInstance();
    static void DestroyInstance();
    static VulkanContext& GetInstance();
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    inline VulkanInstance& GetVulkanInstance() { assert(m_pInstance); return *m_pInstance; }
    inline VulkanPhysicalDevice& GetVulkanPhysicalDevice() { assert(m_pPhysicalDevice); return *m_pPhysicalDevice; }
    inline VulkanDevice& GetVulkanDevice() { assert(m_pDevice); return *m_pDevice; }
    inline VulkanRenderPipeline& GetVulkanRenderPipeline() { assert(m_pRenderPipeline); return *m_pRenderPipeline;}
    inline VulkanDescriptorSets& GetVulkanDescriptorSets() { assert(m_pDescSet); return *m_pDescSet; }
    void Init(const VulkanInstance::Config& instanceConfig,
                const VulkanPhysicalDevice::Config& physicalConfig);
    void Destroy();
private:
    void createVulkanDescriptorSetLayout();
    void createVulkanDescriptorSet();
};

RHI_NAMESPACE_END