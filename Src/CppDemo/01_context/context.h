#pragma once


#include "CppDemo/02_swapchain/swapchain.h"
#include "CppDemo/04_renderprocess/renderprocess.h"
#include <functional>
#include <optional>
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <memory.h>

namespace cpp_demo {

class Context final
{
public:
    struct QueueFamilyIndices final
    {
        std::optional<uint32_t> graphicsQueue;
        std::optional<uint32_t> presentQueue;

        operator bool() const { return graphicsQueue.has_value() && presentQueue.has_value(); }
    };

    using CreateSurfaceFunc = std::function<vk::SurfaceKHR(vk::Instance)>;

private:
    static std::unique_ptr<Context> m_contextInstance;

private:
    vk::Instance m_vkInstance;
    vk::PhysicalDevice m_vkPhysicalDevice;
    vk::Device m_vkDevice;
    vk::Queue m_vkGraphicQueue;
    vk::Queue m_vkPresentQueue;
    vk::SurfaceKHR m_vkSurfaceKHR;
    Context::QueueFamilyIndices m_queueFamilyIndices;
    std::unique_ptr<Swapchain> m_pSwapchain;
    std::unique_ptr<RenderProcess> m_pRenderProcess;

public:
    static Context& Init(const std::vector<const char*>& extensions,
                        CreateSurfaceFunc createSurfaceFunc);
    static void Quit();
    static Context& GetInstance();
    ~Context();

public:

    Context& InitSwapchain(int windowWidth, int windowHeight);
    Context& DestroySwapchain();

    Context& InitRenderProcess(int windowWidth, int windowHeight);
    Context& DestroyRenderProcess();

    inline vk::SurfaceKHR& GetSurface() { return m_vkSurfaceKHR; }
    inline vk::PhysicalDevice& GetPhysicalDevice() { return m_vkPhysicalDevice; }
    inline vk::Device& GetDevice() { return m_vkDevice; }
    inline vk::Queue& GetGraphicQueue() { return m_vkGraphicQueue; }
    inline vk::Queue& GetPresentQueue() { return m_vkPresentQueue; }
    inline Context::QueueFamilyIndices& GetQueueFamilyIndices() { return m_queueFamilyIndices; }
    inline Swapchain& GetSwapchain() { return *m_pSwapchain; }

private:
    void createVkInstance(const std::vector<const char*>& extensions);
    void createVkDevice();
    void getQueues();
    void pickUpPhysicalDevice();

    void queryQueueFamilyIndices();
    explicit Context(const std::vector<const char*>& extensions, CreateSurfaceFunc createSurfaceFunc);
};

int contextDemo();
}