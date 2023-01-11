#include "shader.h"
#include "vulkan/vulkan_structs.hpp"
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
}

Shader::~Shader()
{
    auto& device = Context::GetInstance().GetDevice();
    device.destroyShaderModule(m_vkShaderModuleVertex);
    device.destroyShaderModule(m_vkShaderModuleFragment);
}

}