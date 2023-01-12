#include "shader.h"
#include "Demo/01_createwindow/createwindow.h"
#include "Util/fileutil.h"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include "CppDemo/01_context/context.h"

namespace cpp_demo {


std::unique_ptr<Shader> Shader::m_instance = nullptr;

Shader& Shader::GetInstance()
{
    assert(m_instance);
    return *m_instance;
}

void Shader::Init(const std::string& vertexSource, const std::string& fragSource)
{
    m_instance.reset(new Shader(vertexSource, fragSource));
}
void Shader::Quit()
{
    m_instance.reset();
}

Shader::Shader(const std::string& vertexSource, const std::string& fragSource)
{
    auto& device = Context::GetInstance().GetDevice();
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = vertexSource.size();
    createInfo.pCode = (uint32_t*)vertexSource.data();
    m_vkShaderModuleVertex = device.createShaderModule(createInfo);

    createInfo.codeSize = fragSource.size();
    createInfo.pCode = (uint32_t*)fragSource.data();
    m_vkShaderModuleFragment = device.createShaderModule(createInfo);

    initStage();
}

Shader::~Shader()
{
    auto& device = Context::GetInstance().GetDevice();
    device.destroyShaderModule(m_vkShaderModuleVertex);
    device.destroyShaderModule(m_vkShaderModuleFragment);
}

void Shader::initStage()
{
    m_vkShaderStages.resize(2);
    m_vkShaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex)
                        .setModule(m_vkShaderModuleVertex)
                        .setPName("main");
    m_vkShaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment)
                        .setModule(m_vkShaderModuleFragment)
                        .setPName("main");
}

int shaderDemo(boost::filesystem::path& resourcesPath)
{
    std::cout << "[shaderDemo] resourcesPath: " << resourcesPath << std::endl;
    _01::Window window(_01::WindowSetting{1920, 1080, "Vulkan Shader Demo"});
    window.initWindow();

    Context::Init(window.getRequiredInstanceExtensions(), window.getCreateSurfaceFunc())
            .InitSwapchain(window.getWindowSetting().width, window.getWindowSetting().height);

    auto shaderPath = resourcesPath / "Shader\\GLSL";
    auto vertexShader = shaderPath / "shader.vert";
    auto fragShader = shaderPath / "shader.frag";

    std::string vertShaderSource, fragShaderSource;
    util::file::readFile(vertexShader, vertShaderSource);
    util::file::readFile(fragShader, fragShaderSource);
    Shader::Init(vertShaderSource, fragShaderSource);

    while(!window.shouldClose())
    {
        glfwPollEvents();
    }

    Context::GetInstance().DestroySwapchain()
                            .Quit();
    return 0;
}

}