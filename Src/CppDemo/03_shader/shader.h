#pragma once

#include <boost/filesystem/path.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace cpp_demo {

class Shader final
{
public:

private:
    static std::unique_ptr<Shader> m_instance;

    vk::ShaderModule m_vkShaderModuleVertex;
    vk::ShaderModule m_vkShaderModuleFragment;
public:
    static void Init(const std::string& vertexSource, const std::string& fragSource);
    static void Quit();

    static Shader& GetInstance();

    ~Shader();
private:
    explicit Shader(const std::string& vertexSource, const std::string& fragSource);
};

int shaderDemo(boost::filesystem::path& resourcesPath);
}