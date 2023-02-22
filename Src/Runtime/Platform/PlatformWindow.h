#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "Runtime/Platform/PlatformInputMonitor.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
namespace platform {

struct PlatformWindowSetting
{
    int width;
    int height;
    const char* name;
};

class PlatformWindow
{
protected:
    PlatformWindowSetting m_setting;
    std::unique_ptr<PlatformInputMonitor> m_inputMonitor;
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

    virtual void AddFrameBufferSizeChangedCallback(std::function<void(int, int)> func) = 0;
    virtual void WaitIfMinimization() = 0;
    virtual void PollWindowEvent() = 0;

    inline PlatformInputMonitor* GetInputMonitor() { return m_inputMonitor.get(); }
};

std::unique_ptr<PlatformWindow> CreatePlatformWindow(int width, int height, const char* title);
}