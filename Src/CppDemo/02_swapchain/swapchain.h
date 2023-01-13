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
    std::vector<vk::Image> m_vkImages;
    std::vector<vk::ImageView> m_vkImageViews;
    std::vector<vk::Framebuffer> m_vkFramebuffers;

    vk::SwapchainKHR m_vkSwapchain;
    SwapchainInfo m_swapchainInfo;
public:
    Swapchain(int windowWidth, int windowHeight);
    ~Swapchain();
    inline const SwapchainInfo& GetSwapchainInfo() { return m_swapchainInfo; }
    inline vk::SwapchainKHR& GetSwapchain() { return m_vkSwapchain; }
    inline std::vector<vk::Framebuffer>& GetFramebuffers() { return m_vkFramebuffers; }
    inline vk::Framebuffer& GetFramebuffer(int index) { assert(index >= 0 && index < m_vkFramebuffers.size()); return m_vkFramebuffers[index]; }

    void CreateFrameBuffers(int windowWidth, int windowHeight);
private:
    void queryInfo(int windowWidth, int windowHeight);
    void getImages();
    void createImageViews();

};

}