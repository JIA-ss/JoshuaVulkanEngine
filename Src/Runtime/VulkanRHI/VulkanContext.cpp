#include "VulkanContext.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
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
}

void VulkanContext::Destroy()
{
    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}