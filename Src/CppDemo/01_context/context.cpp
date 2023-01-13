#include "context.h"
#include "CppDemo/02_swapchain/swapchain.h"
#include "CppDemo/04_renderprocess/renderprocess.h"
#include "Demo/01_createwindow/createwindow.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <memory>
#include <stdexcept>


std::string to_string(vk::PhysicalDeviceType type)
{
#define VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(_TYPE_) \
    case vk::PhysicalDeviceType::_TYPE_: return #_TYPE_;

    switch (type)
    {
        VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(eOther)
        VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(eCpu)
        VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(eDiscreteGpu)
        VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(eIntegratedGpu)
        VK_PHYSICAL_DEVICE_TYPE_STRING_CASE(eVirtualGpu)
    }
#undef VK_PHYSICAL_DEVICE_TYPE_STRING_CASE
    return "";
}

namespace cpp_demo {

std::unique_ptr<Context> Context::m_contextInstance = nullptr;


Context::Context(const std::vector<const char*>& extensions,
                    CreateSurfaceFunc createSurfaceFunc)
{
    createVkInstance(extensions);
    pickUpPhysicalDevice();
    m_vkSurfaceKHR = createSurfaceFunc(m_vkInstance);
    if (!m_vkSurfaceKHR)
    {
        throw std::runtime_error("create surface failed");
    }
    queryQueueFamilyIndices();
    createVkDevice();
    getQueues();
}

Context::~Context()
{
    m_vkInstance.destroySurfaceKHR(m_vkSurfaceKHR);
    m_vkDevice.destroy();
    m_vkInstance.destroy();
}

Context& Context::Init(const std::vector<const char*>& extensions, CreateSurfaceFunc createSurfaceFunc)
{
    m_contextInstance.reset(new Context(extensions, createSurfaceFunc));
    return *m_contextInstance;
}

Context& Context::InitSwapchain(int windowWidth, int windowHeight)
{
    assert(!m_pSwapchain);
    m_pSwapchain.reset(new Swapchain(windowWidth, windowHeight));
    return *m_contextInstance;
}

Context& Context::DestroySwapchain()
{
    assert(m_pSwapchain);
    m_pSwapchain.reset();
    return *m_contextInstance;
}

Context& Context::InitRenderProcess(int windowWidth, int windowHeight)
{
    assert(!m_pRenderProcess);
    m_pRenderProcess.reset(new RenderProcess());
    m_pRenderProcess->Init(windowWidth, windowHeight);
    return *m_contextInstance;
}
Context& Context::DestroyRenderProcess()
{
    assert(m_pRenderProcess);
    m_pRenderProcess->Destroy();
    m_pRenderProcess.reset();
    return *m_contextInstance;
}

Context& Context::InitRenderer()
{
    assert(!m_pRenderer);
    m_pRenderer.reset(new Renderer());
    return *m_contextInstance;
}
Context& Context::DestroyRenderer()
{
    assert(m_pRenderer);
    m_pRenderer.reset();
    return *m_contextInstance;
}

void Context::Quit()
{
    m_contextInstance.reset();
}

Context& Context::GetInstance()
{
    assert(m_contextInstance);
    return *m_contextInstance;
}

void Context::createVkInstance(const std::vector<const char*>& extensions)
{
#ifndef NDEBUG
    std::cout << "=== === SUPPORT LAYERS === ===" << std::endl;
    // output all support layers
    auto supported_layers = vk::enumerateInstanceLayerProperties();
    for(auto& layer: supported_layers)
    {
        std::cout << "support layerName:\t" << layer.layerName
                    << "\t("<< layer.description << ")" << std::endl;
    }
    std::cout << "=== === === === === === === ===" << std::endl;

    std::cout << "=== === REQUIRED EXTENSIONS === ===" << std::endl;
    for(auto& extension: extensions)
    {
        std::cout << "required extension:\t" << extension << std::endl;
    }
    std::cout << "=== === === === === === === ===" << std::endl;
#endif

    static std::vector<const char*> attach_layers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    vk::InstanceCreateInfo createInfo;
    vk::ApplicationInfo appInfo;
    appInfo.setApiVersion(VK_API_VERSION_1_2);
    createInfo.setPApplicationInfo(&appInfo)
                .setPEnabledLayerNames(attach_layers)
                .setPEnabledExtensionNames(extensions);

    m_vkInstance = vk::createInstance(createInfo);
}

void Context::pickUpPhysicalDevice()
{
    auto devices = m_vkInstance.enumeratePhysicalDevices();
#ifndef NDEBUG
    std::cout << "=== === PHYSICAL DEVICES === ===" << std::endl;
    // output all physical devices
    for(auto& device: devices)
    {
        auto prop = device.getProperties();
        std::cout << "physical device:\t" << prop.deviceName << "\t" << ::to_string(prop.deviceType) << std::endl;
    }
    std::cout << "=== === === === === === === ===" << std::endl;
    assert(!devices.empty());
#endif
    m_vkPhysicalDevice = devices.front();
}

void Context::createVkDevice()
{
    std::array<const char*, 1> extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::DeviceCreateInfo createInfo;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    float priorities = 1.0;
    if (m_queueFamilyIndices.graphicsQueue.value() == m_queueFamilyIndices.presentQueue.value())
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.graphicsQueue.value());
        queueCreateInfos.emplace_back(queueCreateInfo);
    }
    else
    {
        vk::DeviceQueueCreateInfo graphicQueueCreateInfo;
        graphicQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.graphicsQueue.value());
        queueCreateInfos.emplace_back(graphicQueueCreateInfo);

        vk::DeviceQueueCreateInfo presentQueueCreateInfo;
        presentQueueCreateInfo.setPQueuePriorities(&priorities)
                        .setQueueCount(1)
                        .setQueueFamilyIndex(m_queueFamilyIndices.presentQueue.value());
        queueCreateInfos.emplace_back(presentQueueCreateInfo);
    }
    createInfo.setQueueCreateInfos(queueCreateInfos)
                .setPEnabledExtensionNames(extensions);

    m_vkDevice = m_vkPhysicalDevice.createDevice(createInfo);
}

void Context::queryQueueFamilyIndices()
{
    auto props = m_vkPhysicalDevice.getQueueFamilyProperties();
    for (int i = 0; i < props.size(); i++)
    {
        const auto& property = props[i];
        if (property.queueFlags | vk::QueueFlagBits::eGraphics)
        {
            m_queueFamilyIndices.graphicsQueue = i;
        }

        if (m_vkPhysicalDevice.getSurfaceSupportKHR(i, m_vkSurfaceKHR))
        {
            m_queueFamilyIndices.presentQueue = i;
        }

        if (m_queueFamilyIndices)
        {
            break;
        }
    }
    assert(m_queueFamilyIndices);
}

void Context::getQueues()
{
    assert(m_queueFamilyIndices);
    m_vkGraphicQueue = m_vkDevice.getQueue(m_queueFamilyIndices.graphicsQueue.value(), 0);
    m_vkPresentQueue = m_vkDevice.getQueue(m_queueFamilyIndices.presentQueue.value(), 0);
}

int contextDemo()
{
    _01::Window window({1024,720,"VulkanCpp"});
    window.initWindow();

    Context::Init(
        window.getRequiredInstanceExtensions(),
        window.getCreateSurfaceFunc()
    );

    Context::GetInstance().InitSwapchain(
        window.getWindowSetting().width,
        window.getWindowSetting().height
    );

    while(!window.shouldClose())
    {
        glfwPollEvents();
    }

    Context::Quit();
    return 0;
}

}