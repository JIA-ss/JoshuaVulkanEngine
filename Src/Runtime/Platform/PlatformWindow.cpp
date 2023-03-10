#include "PlatformWindow.h"
#include "Windows/WindowsWindow.h"
#include <memory>

std::unique_ptr<platform::PlatformWindow> platform::CreatePlatformWindow(PlatformWindowSetting setting)
{
#if defined (WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    std::unique_ptr<platform::PlatformWindow> window =
        std::make_unique<platform::WindowsWindow>(setting);
    return window;
#endif
    return nullptr;
}