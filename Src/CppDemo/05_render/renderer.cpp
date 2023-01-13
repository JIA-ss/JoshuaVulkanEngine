#include "renderer.h"
#include "CppDemo/01_context/context.h"
#include "CppDemo/03_shader/shader.h"
#include "Demo/01_createwindow/createwindow.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <limits>
#include <stdexcept>


namespace cpp_demo {

Renderer::Renderer()
{
    initCmdPool();
    allocCmdBuf();
    createVkFence();
    createVkSems();
}

Renderer::~Renderer()
{
    auto& device = Context::GetInstance().GetDevice();
    device.destroySemaphore(m_vkSemImgAvaliable);
    device.destroySemaphore(m_vkSemImgDrawFinish);
    device.destroyFence(m_vkFenceCmd);
    device.freeCommandBuffers(m_vkCmdPool, m_vkCmdBuf);
    device.destroyCommandPool(m_vkCmdPool);
}

void Renderer::initCmdPool()
{
    vk::CommandPoolCreateInfo createInfo;
    createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    m_vkCmdPool = Context::GetInstance().GetDevice().createCommandPool(createInfo);
}

void Renderer::allocCmdBuf()
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandPool(m_vkCmdPool)
                .setCommandBufferCount(1)
                .setLevel(vk::CommandBufferLevel::ePrimary);
    m_vkCmdBuf = Context::GetInstance().GetDevice().allocateCommandBuffers(allocInfo)[0];
}

void Renderer::createVkFence()
{
    vk::FenceCreateInfo createInfo;
    m_vkFenceCmd = Context::GetInstance().GetDevice().createFence(createInfo);
}

void Renderer::createVkSems()
{
    vk::SemaphoreCreateInfo createInfo;
    m_vkSemImgAvaliable = Context::GetInstance().GetDevice().createSemaphore(createInfo);
    m_vkSemImgDrawFinish = Context::GetInstance().GetDevice().createSemaphore(createInfo);
}

void Renderer::Render()
{
    auto& device = Context::GetInstance().GetDevice();
    auto& renderProcess = Context::GetInstance().GetRenderProcess();
    auto& swapchain = Context::GetInstance().GetSwapchain();


    auto result = device.acquireNextImageKHR(
            Context::GetInstance().GetSwapchain().GetSwapchain(), 
            std::numeric_limits<uint64_t>::max(), m_vkSemImgAvaliable);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("acquire next image failed");
    }

    auto imageIndex = result.value;
    m_vkCmdBuf.reset();
    //device.resetCommandPool(m_vkCmdPool);

    vk::CommandBufferBeginInfo begin;
    begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmdBuf.begin(begin);
    {
        m_vkCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, renderProcess.GetPipeline());

        vk::RenderPassBeginInfo renderpassBegin;
        vk::Rect2D area {{0,0}, swapchain.GetSwapchainInfo().imageExtent};
        vk::ClearValue clearValue;
        clearValue.color = vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 0.1f});
        renderpassBegin.setRenderPass(renderProcess.GetRenderPass())
                        .setRenderArea(area)
                        .setFramebuffer(swapchain.GetFramebuffer(imageIndex))
                        .setClearValues(clearValue);
        m_vkCmdBuf.beginRenderPass(renderpassBegin, {});
        {
            m_vkCmdBuf.draw(3, 1, 0, 0);
        }
        m_vkCmdBuf.endRenderPass();
    }
    m_vkCmdBuf.end();

    vk::SubmitInfo submit;
    submit.setCommandBuffers(m_vkCmdBuf)
            .setWaitSemaphores(m_vkSemImgAvaliable)
            .setSignalSemaphores(m_vkSemImgDrawFinish);
    Context::GetInstance().GetGraphicQueue().submit(submit, m_vkFenceCmd);


    vk::PresentInfoKHR present;
    present.setImageIndices(imageIndex)
            .setSwapchains(swapchain.GetSwapchain())
            .setWaitSemaphores(m_vkSemImgDrawFinish);
    if (Context::GetInstance().GetPresentQueue().presentKHR(present) != vk::Result::eSuccess)
    {
        throw std::runtime_error("image present failed");
    }


    if (device.waitForFences(m_vkFenceCmd, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
    {
        throw std::runtime_error("wait for fence failed");
    }
    device.resetFences(m_vkFenceCmd);
}


int rendererDemo(const std::string& vertexSource, const std::string& fragSource)
{
    _01::Window window({1920,1080,"Vulkan Renderer Demo"});
    window.initWindow();

    auto& context = Context::Init(window.getRequiredInstanceExtensions(), window.getCreateSurfaceFunc())
                                    .InitSwapchain(1920, 1080);
    Shader::Init(vertexSource, fragSource);
    context.InitRenderProcess(1920, 1080)
            .InitRenderer();

    context.GetSwapchain().CreateFrameBuffers(1920, 1080);

    while(!window.shouldClose())
    {
        glfwPollEvents();
        context.GetRenderer().Render();
    }
    context.GetDevice().waitIdle();
    Shader::Quit();
    context.DestroyRenderer()
            .DestroyRenderProcess()
            .DestroySwapchain()
            .Quit();

    return 0;
}
}