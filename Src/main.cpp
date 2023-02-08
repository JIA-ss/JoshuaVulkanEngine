#include "CppDemo/03_shader/shader.h"
#include "CppDemo/04_renderprocess/renderprocess.h"
#include "CppDemo/05_render/renderer.h"
#include "Demo/01_createwindow/createwindow.h"
#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "Demo/03_physicalDeviceAndQueue/physicalDeviceAndQueue.h"

#include "CppDemo/01_context/context.h"
#include "Runtime/Platform/PlatformWindow.h"
#include "Runtime/Render/Renderer.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Util/fileutil.h"
#include <GLFW/glfw3.h>
#include <boost/filesystem/path.hpp>

#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_core.h"


std::unique_ptr<platform::PlatformWindow> window = nullptr;

void StartUp(const boost::filesystem::path& exePath, const boost::filesystem::path& resourcesPath)
{
    window = platform::CreatePlatformWindow(1920 / 2, 1080 / 2, "RHI");
    window->Init();
    auto extensions = window->GetRequiredExtensions();
    std::vector<const char*> enabledInstanceExtensions;

    vk::PhysicalDeviceFeatures feature;
    feature.setSamplerAnisotropy(VK_TRUE);
    auto& ctx = RHI::VulkanContext::CreateInstance();
    ctx.Init(
        RHI::VulkanInstance::Config { true, "RHI", "RHI", VK_API_VERSION_1_2, extensions },
        RHI::VulkanPhysicalDevice::Config { window.get(), feature }
        );

}

void Run()
{
    Render::Renderer rd;
    while(!window->ShouldClose())
    {
        glfwPollEvents();
        rd.Render();
    }
}

void Shutdown()
{
    RHI::VulkanContext::GetInstance().Destroy();
    window->Destroy();
    RHI::VulkanContext::DestroyInstance();
    window.reset();
}


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "launch args must specify path of Resources" << std::endl;
        return -1;
    }

    boost::filesystem::path exePath = argv[0];
    boost::filesystem::path resourcesPath = argv[1];
    util::file::setExePath(exePath);
    util::file::setResourcePath(resourcesPath);
#ifndef NDEBUG
    std::cout << "exePath: " << exePath << std::endl;
    std::cout << "resourcesPath: " << resourcesPath << std::endl;
#endif
    //return _01::createWindow();
    //return _02::createVulkanInstance();
    //return _03::physicalDeviceAndQueue();
    // return cpp_demo::contextDemo();
    //return cpp_demo::shaderDemo(resourcesPath);
    std::string vertexSource, fragSource;
    util::file::readFile(resourcesPath/"Shader\\GLSL\\SPRI-V\\shader.vert.spv", vertexSource, util::file::eFileOpenMode::kBinary);
    util::file::readFile(resourcesPath/"Shader\\GLSL\\SPRI-V\\shader.frag.spv", fragSource, util::file::eFileOpenMode::kBinary);
    //return cpp_demo::renderProcessDemo(vertexSource, fragSource);
    //return cpp_demo::rendererDemo(vertexSource, fragSource);

    // _01::Window window({1920,1080,"demo"});
    // window.initWindow();


    StartUp(exePath, resourcesPath);

    Run();

    Shutdown();
}