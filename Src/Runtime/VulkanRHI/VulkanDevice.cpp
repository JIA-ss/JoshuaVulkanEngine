#include "VulkanDevice.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanPipelineCache.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string.h>

RHI_NAMESPACE_USING

VulkanDevice::VulkanDevice(VulkanPhysicalDevice* physicalDevice) : m_vulkanPhysicalDevice(physicalDevice)
{
    std::cout << "=== === === VulkanDevice Construct Begin === === ===" << std::endl;

    assert(physicalDevice);

    m_vkSurfaceKHR = m_vulkanPhysicalDevice->GetPVkSurface();
    m_queueFamilyIndices = m_vulkanPhysicalDevice->GetPQueueFamilyIndices();

    vk::DeviceCreateInfo createInfo;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo;
    std::vector<const char*> enableExtensions = m_vulkanPhysicalDevice->GetConfig().requiredExtensions;
    setUpQueueCreateInfos(createInfo, queueInfo);
    setUpExtensions(createInfo, enableExtensions);
    if (m_vulkanPhysicalDevice->GetConfig().requiredFeatures)
    {
        createInfo.setPEnabledFeatures(&m_vulkanPhysicalDevice->GetConfig().requiredFeatures.value());
    }
    m_vkDevice = m_vulkanPhysicalDevice->GetVkPhysicalDevice().createDevice(createInfo);

    m_vkGraphicQueue = m_vkDevice.getQueue(m_queueFamilyIndices->graphic.value(), 0);
    m_vkPresentQueue = m_vkDevice.getQueue(m_queueFamilyIndices->present.value(), 0);

    m_pVulkanCmdPool.reset(new VulkanCommandPool(this, m_queueFamilyIndices->graphic.value()));
    m_pVulkanSwapchain.reset(new VulkanSwapchain(this));
    m_pVulkanPipelineCache.reset(new VulkanPipelineCache(this, m_vulkanPhysicalDevice->GetPhysicalDeviceInfo().deviceProps,
        Util::File::getResourcePath() / "PipelineCache\\pipelinecache.bin"));

    m_VulkanDescriptorSetLayoutPresets.Init(this);
    std::cout << "=== === === VulkanDevice Construct End === === ===" << std::endl;
}

VulkanDevice::~VulkanDevice()
{
    m_vkDevice.waitIdle();
    m_pVulkanFramebuffers.clear();
    for (auto& presentFramebuffer : m_pPresentVulkanFramebuffers)
    {
        presentFramebuffer.reset();
    }
    m_VulkanDescriptorSetLayoutPresets.UnInit();
    m_pVulkanPipelineCache.reset();
    m_pVulkanSwapchain.reset();
    m_pVulkanCmdPool.reset();
    m_vkDevice.destroy();
}

void VulkanDevice::setUpQueueCreateInfos(vk::DeviceCreateInfo& createInfo, std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfos)
{
    std::array<const char*, 1> extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float priorities = 1.0;
    if (m_queueFamilyIndices->graphic.value() == m_queueFamilyIndices->present.value())
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices->graphic.value());
        queueCreateInfos.emplace_back(queueCreateInfo);
    }
    else
    {
        vk::DeviceQueueCreateInfo graphicQueueCreateInfo;
        graphicQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices->graphic.value());
        queueCreateInfos.emplace_back(graphicQueueCreateInfo);

        vk::DeviceQueueCreateInfo presentQueueCreateInfo;
        presentQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices->present.value());
        queueCreateInfos.emplace_back(presentQueueCreateInfo);
    }
    createInfo.setQueueCreateInfos(queueCreateInfos);


}

void VulkanDevice::setUpExtensions(vk::DeviceCreateInfo& createInfo, std::vector<const char*>& enableExtensions)
{

    if (m_vulkanPhysicalDevice->SupportExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
    {
        enableExtensions.emplace_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }

    // swap chain
    auto it = std::find_if(enableExtensions.begin(), enableExtensions.end(), 
            [](const char* extension){return strcmp(extension, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;}
    );
    if (it == enableExtensions.end())
    {
        enableExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    for (auto& extension : enableExtensions)
    {
        if (!m_vulkanPhysicalDevice->SupportExtension(extension))
        {
            std::cerr << "Enable device extension \"" << extension << "\" is not present at device level\n";
        }
    }

    createInfo.setPEnabledExtensionNames(enableExtensions);

#ifndef NDEBUG
    std::cout << "[Device Enable Extensions]" << std::endl;
    std::transform(enableExtensions.begin(), enableExtensions.end(), enableExtensions.begin(), 
    [](const char* ext) { std::cout << ext << "\t"; return ext; });
    std::cout << std::endl;
#endif
}


std::vector<vk::SurfaceFormatKHR> VulkanDevice::GetSurfaceFormat()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfaceFormatsKHR(*m_vkSurfaceKHR);
}

vk::SurfaceCapabilitiesKHR VulkanDevice::GetSurfaceCapabilities()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfaceCapabilitiesKHR(*m_vkSurfaceKHR);
}

std::vector<vk::PresentModeKHR> VulkanDevice::GetSurfacePresentMode()
{
    return m_vulkanPhysicalDevice->GetVkPhysicalDevice().getSurfacePresentModesKHR(*m_vkSurfaceKHR);;
}

VulkanFramebuffer* VulkanDevice::CreateVulkanFramebuffer(const char* name, VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer,
        const std::vector<VulkanFramebuffer::Attachment>& attachments
)
{
    assert(m_pVulkanFramebuffers.find(name) == m_pVulkanFramebuffers.end());
    m_pVulkanFramebuffers[name].reset(new VulkanFramebuffer(this, renderpass, width, height, layer, attachments));
    return m_pVulkanFramebuffers[name].get();
}

void VulkanDevice::DestroyVulkanFramebuffer(const char* name)
{
    m_pVulkanFramebuffers.erase(name);
}

VulkanFramebuffer* VulkanDevice::GetVulkanFramebuffer(const char* name)
{
    auto it = m_pVulkanFramebuffers.find(name);
    if (it != m_pVulkanFramebuffers.end())
    {
        return it->second.get();
    }
    assert(false);
    return nullptr;
}

void VulkanDevice::CreateVulkanPresentFramebuffer(VulkanRenderPass* renderpass,
        uint32_t width, uint32_t height, uint32_t layer,
        const std::vector<VulkanFramebuffer::Attachment>& attachments,
        int presentImageIdx
)
{
    auto attachmentsCopy = attachments;
    auto presentImages = m_pVulkanSwapchain->GetVulkanPresentImages();
    m_pPresentVulkanFramebuffers.resize(presentImages.size());
    for (int i = 0; i < m_pPresentVulkanFramebuffers.size(); i++)
    {
        m_pVulkanSwapchain->GetVulkanPresentColorAttachment(i, attachmentsCopy[presentImageIdx]);
        assert(
            attachmentsCopy[presentImageIdx].resource.vkImageView
            && attachmentsCopy[presentImageIdx].resource.vkImageView.value() == m_pVulkanSwapchain->GetVulkanPresentImage(i).vkImageView.value()
        );
        m_pPresentVulkanFramebuffers[i].reset(new VulkanFramebuffer(this, renderpass, width, height, layer, attachmentsCopy));
    }
}



void VulkanDevice::ReCreateSwapchain(VulkanRenderPass* renderPass,
        uint32_t width, uint32_t height, uint32_t layer,
        std::vector<VulkanFramebuffer::Attachment>& attachments,
        int presentImageIdx)
{
    assert(presentImageIdx < attachments.size());
    m_pVulkanSwapchain.reset();
    m_pVulkanSwapchain.reset(new VulkanSwapchain(this));

    m_pVulkanSwapchain->GetVulkanPresentColorAttachment(0, attachments[presentImageIdx]);
    CreateVulkanPresentFramebuffer(renderPass, width, height, layer, attachments);
}