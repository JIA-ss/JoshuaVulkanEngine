#pragma once



#include <boost/filesystem/path.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vector>

namespace cpp_demo {

class Shader final
{
public:

private:
    static std::unique_ptr<Shader> m_instance;

    vk::ShaderModule m_vkShaderModuleVertex;
    vk::ShaderModule m_vkShaderModuleFragment;
    std::vector<vk::PipelineShaderStageCreateInfo> m_vkShaderStages;
public:
    static void Init(const std::string& vertexSource, const std::string& fragSource);
    static void Quit();

    static Shader& GetInstance();
public:
    inline vk::ShaderModule& GetVertexModule() { return m_vkShaderModuleVertex; }
    inline vk::ShaderModule& GetFragmentModule() { return m_vkShaderModuleFragment; }
    inline std::vector<vk::PipelineShaderStageCreateInfo>& GetStage() { return m_vkShaderStages; }
    ~Shader();
private:
    explicit Shader(const std::string& vertexSource, const std::string& fragSource);
    void initStage();
};

int shaderDemo(boost::filesystem::path& resourcesPath);
}