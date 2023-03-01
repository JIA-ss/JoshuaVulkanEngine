#include "CppDemo/03_shader/shader.h"
#include "CppDemo/04_renderprocess/renderprocess.h"
#include "CppDemo/05_render/renderer.h"
#include "Demo/01_createwindow/createwindow.h"
#include "Demo/02_createVulkanInstance/createVulkanInstance.h"
#include "Demo/03_physicalDeviceAndQueue/physicalDeviceAndQueue.h"

#include "CppDemo/01_context/context.h"
#include "Runtime/Platform/PlatformWindow.h"
#include "Runtime/Render/MultiPipelines/MultiPipelineRenderer.h"
#include "Runtime/Render/Renderer.h"
#include "Runtime/Render/RendererBase.h"
#include "Runtime/Render/SimpleModel/SimpleModelRenderer.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include "Util/Fileutil.h"
#include <GLFW/glfw3.h>
#include <boost/filesystem/path.hpp>

#include "Runtime/VulkanRHI/VulkanContext.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"


std::unique_ptr<platform::PlatformWindow> window = nullptr;
std::shared_ptr<Render::RendererBase> render = nullptr;

void StartUp(const boost::filesystem::path& exePath, const boost::filesystem::path& resourcesPath, const std::string& demoName)
{

    window = platform::CreatePlatformWindow(1920, 1080, "RHI");
    window->Init();
    auto extensions = window->GetRequiredExtensions();
    std::vector<const char*> enabledInstanceExtensions;

    vk::PhysicalDeviceFeatures feature;
    feature.setSamplerAnisotropy(VK_TRUE)
            .setFillModeNonSolid(VK_TRUE);

    render = Render::RendererBase::StartUpRenderer(
        demoName,
        RHI::VulkanInstance::Config { true, "RHI", "RHI", VK_API_VERSION_1_2, extensions },
        RHI::VulkanPhysicalDevice::Config { window.get(), feature, {}, vk::SampleCountFlagBits::e1 }
    );
}

void Run()
{
    render->RenderLoop();
}

void Shutdown()
{
    render.reset();
    window->Destroy();
    window.reset();
}


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "launch args must specify path of Resources" << std::endl;
        return -1;
    }
    if (argc < 3)
    {
        std::cout << "launch args must specify demo name" << std::endl;
        return -1;
    }

    boost::filesystem::path exePath = argv[0];
    boost::filesystem::path resourcesPath = argv[1];
    Util::File::setExePath(exePath);
    Util::File::setResourcePath(resourcesPath);
    std::string demoName = argv[2];
#ifndef NDEBUG
    std::cout << "exePath: " << exePath << std::endl;
    std::cout << "resourcesPath: " << resourcesPath << std::endl;
    std::cout << "demoName: " << demoName <<  std::endl;
#endif
    //return _01::createWindow();
    //return _02::createVulkanInstance();
    //return _03::physicalDeviceAndQueue();
    // return cpp_demo::contextDemo();
    //return cpp_demo::shaderDemo(resourcesPath);
    std::string vertexSource, fragSource;
    Util::File::readFile(resourcesPath/"Shader\\GLSL\\SPRI-V\\shader.vert.spv", vertexSource, Util::File::eFileOpenMode::kBinary);
    Util::File::readFile(resourcesPath/"Shader\\GLSL\\SPRI-V\\shader.frag.spv", fragSource, Util::File::eFileOpenMode::kBinary);
    //return cpp_demo::renderProcessDemo(vertexSource, fragSource);
    //return cpp_demo::rendererDemo(vertexSource, fragSource);

    // _01::Window window({1920,1080,"demo"});
    // window.initWindow();


    StartUp(exePath, resourcesPath, demoName);

    Run();

    Shutdown();
}