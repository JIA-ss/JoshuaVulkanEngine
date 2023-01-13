#pragma once

#include <vulkan/vulkan.hpp>

namespace cpp_demo {

class Renderer final
{
public:
private:
    vk::CommandPool m_vkCmdPool;
    vk::CommandBuffer m_vkCmdBuf;
    vk::Fence m_vkFenceCmd;
    vk::Semaphore m_vkSemImgAvaliable;
    vk::Semaphore m_vkSemImgDrawFinish;
public:
    Renderer();
    ~Renderer();

    void Render();

private:
    void initCmdPool();
    void allocCmdBuf();
    void createVkFence();
    void createVkSems();
};

int rendererDemo(const std::string& vertexSource, const std::string& fragSource);
}