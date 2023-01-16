#include "VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_handles.hpp"
#include <iostream>
RHI_NAMESPACE_USING

VulkanPhysicalDevice::VulkanPhysicalDevice(const Config& config, VulkanInstance* instance)
{
    m_window = config.window;
    m_pVulkanInstance = instance;

    auto devices = m_pVulkanInstance->GetVkInstance().enumeratePhysicalDevices();
    if (devices.empty())
    {
        std::cerr << "No device with Vulkan support found\n";
        assert(false);
        return;
    }

    m_vkPhysicalDevice = devices.front();
    m_physicalDeviceInfo.deviceProps = m_vkPhysicalDevice.getProperties();
    m_physicalDeviceInfo.deviceFeatures = m_vkPhysicalDevice.getFeatures();
    m_physicalDeviceInfo.deviceMemoryProps = m_vkPhysicalDevice.getMemoryProperties();
    m_physicalDeviceInfo.deviceQueueFamilyProps = m_vkPhysicalDevice.getQueueFamilyProperties();
    m_physicalDeviceInfo.deviceExtensionProps = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();
    for (auto& extension : m_physicalDeviceInfo.deviceExtensionProps)
    {
        m_physicalDeviceInfo.supportedExtensions.emplace_back(extension.extensionName);
    }


    std::cout << "Device: " << m_physicalDeviceInfo.deviceProps.deviceName << std::endl;
    std::cout << "API: "
                            << (m_physicalDeviceInfo.deviceProps.apiVersion >> 22)
                            << "."
                            << ((m_physicalDeviceInfo.deviceProps.apiVersion >> 12) & 0x3ff)
                            << "."
                            << (m_physicalDeviceInfo.deviceProps.apiVersion & 0xfff)
                        << std::endl;

	// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	// getEnabledFeatures();
}

vk::SurfaceKHR VulkanPhysicalDevice::CreateVkSurface()
{
    if (m_window)
    {
        return m_window->CreateSurface(m_pVulkanInstance);
    }
    return {};
}

void VulkanPhysicalDevice::DestroyVkSurface(vk::SurfaceKHR& surface)
{
    if (m_window)
    {
        m_window->DestroySurface(m_pVulkanInstance, &surface);
    }
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{

}

VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::QueryQueueFamilyIndices(vk::SurfaceKHR& surface)
{
    VulkanPhysicalDevice::QueueFamilyIndices queueIndices;
    auto props = m_physicalDeviceInfo.deviceQueueFamilyProps;
    for (int i = 0; i < props.size(); i++)
    {
        const auto& property = props[i];
        if (!queueIndices.graphic.has_value() && (property.queueFlags & vk::QueueFlagBits::eGraphics))
        {
            queueIndices.graphic = i;
        }

        if (!queueIndices.present.has_value() && (m_vkPhysicalDevice.getSurfaceSupportKHR(i, surface)))
        {
            queueIndices.present = i;
        }

        if (queueIndices)
        {
            break;
        }
    }
    assert(queueIndices);
    return queueIndices;
}

bool VulkanPhysicalDevice::SupportExtension(const std::string& extension)
{
    return std::find(m_physicalDeviceInfo.supportedExtensions.begin(),
                        m_physicalDeviceInfo.supportedExtensions.end(), 
                        extension)
                != m_physicalDeviceInfo.supportedExtensions.end();
}
