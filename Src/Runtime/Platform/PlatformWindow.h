#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
namespace platform {

struct PlatformWindowSetting
{
    unsigned int width;
    unsigned int height;
    const char* name;
};

class PlatformWindow
{
protected:
    PlatformWindowSetting m_setting;
public:
    explicit PlatformWindow(const PlatformWindowSetting& setting) : m_setting(setting) { };
    virtual ~PlatformWindow() { }

    virtual void Init() = 0;
    virtual void Destroy() = 0;
    virtual bool ShouldClose() = 0;
    virtual void* GetRawHandler() = 0;

    virtual vk::SurfaceKHR CreateSurface(RHI::VulkanInstance* instance) = 0;
    virtual void DestroySurface(RHI::VulkanInstance* instance, vk::SurfaceKHR* surface) = 0;
    
    virtual std::vector<const char*> GetRequiredExtensions() = 0;
    inline const PlatformWindowSetting& GetWindowSetting() { return m_setting; }
};

std::unique_ptr<PlatformWindow> CreatePlatformWindow(unsigned int width, unsigned int height, const char* title);
}