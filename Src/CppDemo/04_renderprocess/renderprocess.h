#pragma once
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
namespace cpp_demo {

class RenderProcess final
{
public:

private:
    vk::Pipeline m_vkPipeline;
    vk::PipelineLayout m_vkPipelineLayout;
    vk::RenderPass m_vkRenderpass;
public:
    ~RenderProcess() = default;
    void Init(int windowWidth, int windowHeight);
    void Destroy();
private:
    void initLayout();
    void initRenderpass();
    void initPipeline(int windowWidth, int windowHeight);
};

int renderProcessDemo(const std::string& vertexSource, const std::string& fragSource);
}