#include "VulkanPhysicalDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"

#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

RHI_NAMESPACE_USING

VulkanPhysicalDevice::VulkanPhysicalDevice(const Config& config, VulkanInstance* instance)
{
    std::cout << "=== === === VulkanPhysicalDevice Construct Begin === === ===" << std::endl;

    m_config = config;
    m_pVulkanInstance = instance;
    createVkSurface();

    auto devices = m_pVulkanInstance->GetVkInstance().enumeratePhysicalDevices();
    if (devices.empty())
    {
        std::cerr << "No device with Vulkan support found\n";
        assert(false);
        return;
    }

    pickUpDevice(devices);
    queryDeviceInfo();

    std::cout << "=== === === VulkanPhysicalDevice Construct End === === ===" << std::endl;
}

void VulkanPhysicalDevice::createVkSurface()
{
    if (m_config.window)
    {
        m_vkSurface = m_config.window->CreateSurface(m_pVulkanInstance);
#ifndef NDEBUG
        std::cout << "[Create Surface]" << std::endl;
#endif
    }
}

void VulkanPhysicalDevice::destroyVkSurface()
{
    if (m_vkSurface && m_config.window)
    {
        m_config.window->DestroySurface(m_pVulkanInstance, &m_vkSurface);
    }
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
    destroyVkSurface();
}

bool VulkanPhysicalDevice::queryQueueFamilyIndices(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface, QueueFamilyIndices& queueIndices)
{
    queueIndices.graphic.reset();
    queueIndices.present.reset();

    auto props = device.getQueueFamilyProperties();
    for (int i = 0; i < props.size(); i++)
    {
        const auto& property = props[i];
        if (!queueIndices.graphic.has_value() && (property.queueFlags & vk::QueueFlagBits::eGraphics))
        {
            queueIndices.graphic = i;
        }

        if (!queueIndices.present.has_value() && (device.getSurfaceSupportKHR(i, surface)))
        {
            queueIndices.present = i;
        }

        if (queueIndices)
        {
            break;
        }
    }
    bool result = queueIndices;
    return result;
}

bool VulkanPhysicalDevice::checkSupportExtension(const vk::PhysicalDevice& device)
{
    auto avaliable_extensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> required;
    std::set<std::string> avaliable;
    std::transform(m_config.requiredExtensions.begin(), m_config.requiredExtensions.end(), m_config.requiredExtensions.begin(),
        [&required](const char* extension) { required.insert(std::string(extension)); return extension; });
    std::transform(avaliable_extensions.begin(), avaliable_extensions.end(), avaliable_extensions.begin(),
        [&avaliable](const vk::ExtensionProperties& prop){ avaliable.insert(std::string(prop.extensionName)); return prop; });

    std::set<std::string> intersection;
    std::set_intersection(required.begin(), required.end(), avaliable.begin(), avaliable.end(), std::inserter(intersection, intersection.begin()));

    if (intersection.size() != required.size())
    {
        std::set<std::string> unsupport;
        std::set_difference(required.begin(), required.end(), intersection.begin(), intersection.end(), std::inserter(unsupport, unsupport.begin()));
        for (auto& unsp : unsupport)
        {
            std::cerr << "unsupport extension: " << unsp << "\n";
        }
        return false;
    }

    return true;
}

bool VulkanPhysicalDevice::checkSupportSwapchain(const vk::PhysicalDevice& device)
{
    // auto capabilities = device.getSurfaceCapabilitiesKHR(m_vkSurface);
    auto formats = device.getSurfaceFormatsKHR(m_vkSurface);
    if (formats.empty())
    {
        return false;
    }
    auto presentMode = device.getSurfacePresentModesKHR(m_vkSurface);
    if (presentMode.empty())
    {
        return false;
    }
    return true;
}

bool VulkanPhysicalDevice::checkSupportFeatures(const vk::PhysicalDevice& device)
{
    if (!m_config.requiredFeatures.has_value())
    {
        return true;
    }
    vk::PhysicalDeviceFeatures avaliableFeatures = device.getFeatures();

    return avaliableFeatures.samplerAnisotropy == m_config.requiredFeatures->samplerAnisotropy;
}

bool VulkanPhysicalDevice::SupportExtension(const std::string& extension)
{
    return std::find(m_physicalDeviceInfo.supportedExtensions.begin(),
                        m_physicalDeviceInfo.supportedExtensions.end(), 
                        extension)
                != m_physicalDeviceInfo.supportedExtensions.end();
}

void VulkanPhysicalDevice::pickUpDevice(const std::vector<vk::PhysicalDevice>& devices)
{
    for (const vk::PhysicalDevice& device : devices)
    {
        if (!queryQueueFamilyIndices(device, m_vkSurface, m_queueFamilyIndices) ||
            !checkSupportExtension(device) ||
            !checkSupportSwapchain(device) ||
            !checkSupportFeatures(device))
        {
            continue;
        }

        m_vkPhysicalDevice = device;
        break;
    }

    if (!m_vkPhysicalDevice)
    {
        std::cerr << "cannot find suitable device\n";
    }
    //m_vkPhysicalDevice = devices.front();
}

void VulkanPhysicalDevice::queryDeviceInfo()
{
    if (!m_vkPhysicalDevice)
    {
        assert(false);
        return;
    }
    
    m_physicalDeviceInfo.deviceProps = m_vkPhysicalDevice.getProperties();
    m_physicalDeviceInfo.deviceMemoryProps = m_vkPhysicalDevice.getMemoryProperties();
    auto avaliableExtensions = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();
    for (auto& extension : avaliableExtensions)
    {
        m_physicalDeviceInfo.supportedExtensions.emplace_back(extension.extensionName);
    }

    vk::SampleCountFlags counts = m_physicalDeviceInfo.deviceProps.limits.framebufferColorSampleCounts &
                                    m_physicalDeviceInfo.deviceProps.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64)
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e64;
    }
    else if (counts & vk::SampleCountFlagBits::e32)
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e32;
    }
    else if (counts & vk::SampleCountFlagBits::e16)
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e16;
    }
    else if (counts & vk::SampleCountFlagBits::e8)
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e8;
    }
    else if (counts & vk::SampleCountFlagBits::e4)
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e4;
    }
    else
    {
        m_physicalDeviceInfo.maxUsableSampleCount = vk::SampleCountFlagBits::e1;
    }


    std::cout << "[Pick Physical Device]" << std::endl
                <<"Name: "  << m_physicalDeviceInfo.deviceProps.deviceName << std::endl
                << "API: "
                            << (m_physicalDeviceInfo.deviceProps.apiVersion >> 22)
                            << "."
                            << ((m_physicalDeviceInfo.deviceProps.apiVersion >> 12) & 0x3ff)
                            << "."
                            << (m_physicalDeviceInfo.deviceProps.apiVersion & 0xfff)
                        << std::endl
                << "MaxUsableSampleCount: " << (int)m_physicalDeviceInfo.maxUsableSampleCount << std::endl;

	// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	// getEnabledFeatures();
}

vk::Format VulkanPhysicalDevice::querySupportFormat(const std::vector<vk::Format>& candidates, const vk::ImageTiling& imgTiling, const vk::FormatFeatureFlagBits& feature)
{
    for(auto& format:candidates)
    {
        auto prop = m_vkPhysicalDevice.getFormatProperties(format);
        switch(imgTiling)
        {
        case vk::ImageTiling::eLinear:
        {
            if ((prop.linearTilingFeatures & feature) == feature)
            {
                return format;
            }
            break;
        }
        case vk::ImageTiling::eOptimal:
        {
            if ((prop.optimalTilingFeatures & feature) == feature)
            {
                return format;
            }
            break;
        }
        default:
        {
            break;
        }
        }
    }

    throw std::runtime_error("can not find supported format");
    return vk::Format::eUndefined;
}

vk::Format VulkanPhysicalDevice::QuerySupportedDepthFormat()
{
    static const std::vector<vk::Format> candidates
    {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint,
    }; 

    static const vk::ImageTiling imageTiling = vk::ImageTiling::eOptimal;

    static const vk::FormatFeatureFlagBits feature = vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    return querySupportFormat(candidates, imageTiling, feature);
}