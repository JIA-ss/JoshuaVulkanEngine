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
    inline vk::Pipeline& GetPipeline() { return m_vkPipeline; }
    inline vk::PipelineLayout& GetPipelineLayout() { return m_vkPipelineLayout; }
    inline vk::RenderPass& GetRenderPass() { return m_vkRenderpass; }

    void Init(int windowWidth, int windowHeight);
    void Destroy();
private:
    void initLayout();
    void initRenderpass();
    void initPipeline(int windowWidth, int windowHeight);
};

int renderProcessDemo(const std::string& vertexSource, const std::string& fragSource);
}