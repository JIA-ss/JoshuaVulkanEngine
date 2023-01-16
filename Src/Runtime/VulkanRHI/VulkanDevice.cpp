#include "VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>

RHI_NAMESPACE_USING

VulkanDevice::VulkanDevice(const Config& config, VulkanPhysicalDevice* physicalDevice) : m_vulkanPhysicalDevice(physicalDevice), m_config(config)
{
    assert(physicalDevice);

    m_vkSurfaceKHR = m_vulkanPhysicalDevice->CreateVkSurface();
    m_queueFamilyIndices = m_vulkanPhysicalDevice->QueryQueueFamilyIndices(m_vkSurfaceKHR);

    vk::DeviceCreateInfo createInfo;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo;
    setUpQueueCreateInfos(createInfo, queueInfo);
    setUpExtensions(createInfo);
    if (m_config.enableFeatures)
    {
        createInfo.setPEnabledFeatures(&m_config.enableFeatures.value());
    }
    m_vkDevice = m_vulkanPhysicalDevice->GetVkPhysicalDevice().createDevice(createInfo);

    m_vkGraphicQueue = m_vkDevice.getQueue(m_queueFamilyIndices.graphic.value(), 0);
    m_vkPresentQueue = m_vkDevice.getQueue(m_queueFamilyIndices.present.value(), 0);

    m_pVulkanSwapchain.reset(new VulkanSwapchain(this));
}

VulkanDevice::~VulkanDevice()
{
    m_pVulkanSwapchain.reset();
    m_vulkanPhysicalDevice->DestroyVkSurface(m_vkSurfaceKHR);
    m_vkDevice.destroy();
}

void VulkanDevice::setUpQueueCreateInfos(vk::DeviceCreateInfo& createInfo, std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfos)
{
    std::array<const char*, 1> extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float priorities = 1.0;
    if (m_queueFamilyIndices.graphic.value() == m_queueFamilyIndices.present.value())
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.graphic.value());
        queueCreateInfos.emplace_back(queueCreateInfo);
    }
    else
    {
        vk::DeviceQueueCreateInfo graphicQueueCreateInfo;
        graphicQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.graphic.value());
        queueCreateInfos.emplace_back(graphicQueueCreateInfo);

        vk::DeviceQueueCreateInfo presentQueueCreateInfo;
        presentQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.present.value());
        queueCreateInfos.emplace_back(presentQueueCreateInfo);
    }
    createInfo.setQueueCreateInfos(queueCreateInfos);


}

void VulkanDevice::setUpExtensions(vk::DeviceCreateInfo& createInfo)
{
    if (m_vulkanPhysicalDevice->SupportExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
    {
        m_config.enableExtensions.emplace_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }

    // swap chain
    m_config.enableExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for (auto& extension : m_config.enableExtensions)
    {
        if (!m_vulkanPhysicalDevice->SupportExtension(extension))
        {
            std::cerr << "Enable device extension \"" << extension << "\" is not present at device level\n";
        }
    }
    createInfo.setPEnabledExtensionNames(m_config.enableExtensions);
}


std::vector<vk::SurfaceFormatKHR> VulkanDevice::GetSurfaceFormat()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfaceFormatsKHR(m_vkSurfaceKHR);
}

vk::SurfaceCapabilitiesKHR VulkanDevice::GetSurfaceCapabilities()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfaceCapabilitiesKHR(m_vkSurfaceKHR);
}

std::vector<vk::PresentModeKHR> VulkanDevice::GetSurfacePresentMode()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfacePresentModesKHR(m_vkSurfaceKHR);;
}