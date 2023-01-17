#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
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
    std::unique_ptr<VulkanShaderSet> m_pShaderSet;
public:
    static VulkanContext& CreateInstance();
    static void DestroyInstance();
    static VulkanContext& GetInstance();
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    inline VulkanInstance& GetVulkanInstance() { assert(m_pInstance); return *m_pInstance; }
    inline VulkanPhysicalDevice& GetVulkanPhysicalDevice() { assert(m_pPhysicalDevice); return *m_pPhysicalDevice; }

    void Init(const VulkanInstance::Config& instanceConfig,
                const VulkanPhysicalDevice::Config& physicalConfig);
    void Destroy();
};

RHI_NAMESPACE_END