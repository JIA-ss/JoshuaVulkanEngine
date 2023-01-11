#pragma once


#include <stdint.h>
#include <vulkan/vulkan.hpp>
namespace cpp_demo {

class Swapchain final
{
public:
    struct SwapchainInfo
    {
        vk::Extent2D imageExtent;
        uint32_t imageCount;
        vk::SurfaceFormatKHR format;
        vk::SurfaceTransformFlagsKHR transform;
        vk::PresentModeKHR present;
    };
private:
    vk::SwapchainKHR m_vkSwapchain;
    SwapchainInfo m_swapchainInfo;
public:
    Swapchain(int windowWidth, int windowHeight);
    ~Swapchain();

private:
    void queryInfo(int windowWidth, int windowHeight);
};

}