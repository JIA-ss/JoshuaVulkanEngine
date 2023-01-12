#include "renderprocess.h"
#include "CppDemo/01_context/context.h"
#include "CppDemo/02_swapchain/swapchain.h"
#include "CppDemo/03_shader/shader.h"
#include "Demo/01_createwindow/createwindow.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <stdint.h>

namespace cpp_demo {

void RenderProcess::Init(int windowWidth, int windowHeight)
{
    initLayout();
    initRenderpass();
    initPipeline(windowWidth, windowHeight);
}
void RenderProcess::Destroy()
{
    auto& device = Context::GetInstance().GetDevice();
    device.destroyPipelineLayout(m_vkPipelineLayout);
    device.destroyRenderPass(m_vkRenderpass);
    device.destroyPipeline(m_vkPipeline);
}

void RenderProcess::initLayout()
{
    vk::PipelineLayoutCreateInfo createInfo;
    m_vkPipelineLayout = Context::GetInstance().GetDevice().createPipelineLayout(createInfo);
}

void RenderProcess::initRenderpass()
{
    vk::RenderPassCreateInfo createInfo;
    vk::AttachmentDescription attachDesc;
    attachDesc.setFormat(Context::GetInstance().GetSwapchain().GetSwapchainInfo().format.format)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setSamples(vk::SampleCountFlagBits::e1);

    vk::AttachmentReference reference;
    reference.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setAttachment(0);
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(reference);


    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);


    createInfo.setSubpasses(subpass);
    createInfo.setAttachments(attachDesc);
    createInfo.setDependencies(dependency);
    m_vkRenderpass = Context::GetInstance().GetDevice().createRenderPass(createInfo);
}

void RenderProcess::initPipeline(int windowWidth, int windowHeight)
{
    vk::GraphicsPipelineCreateInfo createInfo;

    // 1. Vertex Input
    vk::PipelineVertexInputStateCreateInfo inputState;
    createInfo.setPVertexInputState(&inputState);

    // 2. Vertex Assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.setPrimitiveRestartEnable(false)
            .setTopology(vk::PrimitiveTopology::eTriangleList);
    createInfo.setPInputAssemblyState(&inputAsm);

    // 3. Shader
    createInfo.setStages(Shader::GetInstance().GetStage());

    // 4. viewport
    vk::PipelineViewportStateCreateInfo viewportState;
    vk::Viewport viewport(0,0, windowWidth, windowHeight, 0, 0);
    vk::Rect2D rect({0,0}, {(uint32_t)windowWidth, (uint32_t)windowHeight});
    viewportState.setViewports(viewport)
                    .setScissors(rect);
    createInfo.setPViewportState(&viewportState);

    // 5. Rasterization
    vk::PipelineRasterizationStateCreateInfo rastInfo;
    rastInfo.setRasterizerDiscardEnable(false)
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setLineWidth(1);
    createInfo.setPRasterizationState(&rastInfo);

    // 6. multisample
    vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.setSampleShadingEnable(false)
                .setRasterizationSamples(vk::SampleCountFlagBits::e1);
    createInfo.setPMultisampleState(&multisample);

    // 7. test - stencil test, depth test

    // 8. color blending
    vk::PipelineColorBlendStateCreateInfo blend;
    vk::PipelineColorBlendAttachmentState attach;
    attach.setBlendEnable(false)
            .setColorWriteMask(vk::ColorComponentFlagBits::eA |
                                vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB |
                                vk::ColorComponentFlagBits::eR);

    blend.setLogicOpEnable(false)
            .setAttachments(attach);
    createInfo.setPColorBlendState(&blend);

    // layout
    createInfo.setLayout(m_vkPipelineLayout);

    // render pass
    createInfo.setRenderPass(m_vkRenderpass);

    auto result = Context::GetInstance().GetDevice().createGraphicsPipeline(nullptr, createInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("create graphics pipeline failed");
    }
    m_vkPipeline = result.value;

}


int renderProcessDemo(const std::string& vertexSource, const std::string& fragSource)
{
    _01::Window window({1920,1080, "Vulkan RenderProcess Demo"});
    window.initWindow();

    auto& context = Context::Init(window.getRequiredInstanceExtensions(), window.getCreateSurfaceFunc())
                                        .InitSwapchain(1920, 1080);
    Shader::Init(vertexSource, fragSource);
    context.InitRenderProcess(1920, 1080);
    context.GetSwapchain().CreateFrameBuffers(1920, 1080);


    while(!window.shouldClose())
    {
        glfwPollEvents();
    }

    Shader::Quit();
    context.DestroyRenderProcess()
            .DestroySwapchain()
            .Quit();
    return 0;
}

}