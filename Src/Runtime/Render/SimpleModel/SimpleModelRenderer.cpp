#include "SimpleModelRenderer.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "vulkan/vulkan_enums.hpp"
#include <Util/Modelutil.h>
#include <Util/Fileutil.h>
#include <Util/Mathutil.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <iostream>
#include <stdint.h>
using namespace Render;

SimpleModelRenderer::SimpleModelRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, physicalConfig)
{

}

SimpleModelRenderer::~SimpleModelRenderer()
{
    m_pDevice->GetVkDevice().waitIdle();
    m_pRenderPass = nullptr;
    m_pModel.reset();
}

void SimpleModelRenderer::prepare()
{
    // prepare descriptor layout
    prepareModel();
    // prepare callback
    prepareInputCallback();
    // prepare renderpass
    prepareRenderpass();
    // prepare pipeline
    preparePipeline();
    // prepare framebuffer
    prepareFrameBuffer();
}

void SimpleModelRenderer::render()
{

    // wait for fence
    if (
        m_pDevice->GetVkDevice().waitForFences(m_vkFences[m_frameIdxInFlight], true, std::numeric_limits<uint64_t>::max())
        !=
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("wait for inflight fence failed");
    }

    // acquire image
    auto res = m_pDevice->GetVkDevice().acquireNextImageKHR(m_pDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliables[m_frameIdxInFlight], nullptr);
    if (res.result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain();
        return;
    }
    if (res.result != vk::Result::eSuccess && res.result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("acquire next image failed");
    }
    m_imageIdx = res.value;

    updateUniformBuf(m_imageIdx);
    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[m_frameIdxInFlight].begin(beginInfo);
    {
        m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "default");

        std::vector<vk::DescriptorSet> tobinding;
        m_pUniformSets->FillToBindedDescriptorSetsVector(tobinding, m_pPipelineLayout.get(), m_frameIdxInFlight);

        std::vector<vk::ClearValue> clears(2);
        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, m_pDevice->GetSwapchainFramebuffer(m_imageIdx));
        {
            auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
            vk::Rect2D rect{{0,0},extent};
            m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
            m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);
            m_pModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);
        }
        m_pRenderPass->End(m_vkCmds[m_frameIdxInFlight]);
    }
    m_vkCmds[m_frameIdxInFlight].end();

    // submit
    std::array<vk::PipelineStageFlags, 1> waitStage { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    auto submitInfo = vk::SubmitInfo()
                    .setWaitSemaphores(m_vkSemaphoreImageAvaliables[m_frameIdxInFlight])
                    .setWaitDstStageMask(waitStage)
                    .setCommandBuffers(m_vkCmds[m_frameIdxInFlight])
                    .setSignalSemaphores(m_vkSemaphoreRenderFinisheds[m_frameIdxInFlight]);
    m_pDevice->GetVkGraphicQueue().submit(submitInfo, m_vkFences[m_frameIdxInFlight]);

    // present
    auto presentInfo = vk::PresentInfoKHR()
                    .setImageIndices(m_imageIdx)
                    .setSwapchains(m_pDevice->GetPVulkanSwapchain()->GetSwapchain())
                    .setWaitSemaphores(m_vkSemaphoreRenderFinisheds[m_frameIdxInFlight]);
    vk::Result presentRes;
    try
    {
        presentRes = m_pDevice->GetVkPresentQueue().presentKHR(presentInfo);
    }
    catch(vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR || m_frameBufferSizeChanged)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
    }
    else if ( presentRes!= vk::Result::eSuccess )
    {
        throw std::runtime_error("present image failed");
    }
}

void SimpleModelRenderer::prepareModel()
{
    m_pModel.reset(new RHI::Model(m_pDevice.get(), Util::File::getResourcePath() / "Model/nanosuit/nanosuit.obj", m_pSet1SamplerSetLayout.lock().get()));
    auto& transformation = m_pModel->GetTransformation();
    transformation.SetPosition(glm::vec3(0.0f, -10.f, -20.f));
    transformation.SetScale(glm::vec3(0.05f));
}

void SimpleModelRenderer::prepareInputCallback()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 40.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));
    auto inputMonitor = m_pPhysicalDevice->GetPWindow()->GetInputMonitor();
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::W, [&](){
        glm::vec3 dir = m_pCamera->GetDirection();
        m_pCamera->GetVPMatrix().Rotate(glm::vec3(5,0,0));
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::S, [&](){
        glm::vec3 dir = m_pCamera->GetDirection();
        m_pCamera->GetVPMatrix().Rotate(glm::vec3(-5,0,0));
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::A, [&](){
        glm::vec3 right = m_pCamera->GetRight();
        m_pCamera->GetVPMatrix().Rotate(glm::vec3(0,5,0));
    });


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::D, [&](){
        glm::vec3 right = m_pCamera->GetRight();
        m_pCamera->GetVPMatrix().Rotate(glm::vec3(0,-5,0));
    });

    inputMonitor->AddScrollCallback([&](double xoffset, double yoffset){
        glm::vec3 dir = m_pCamera->GetDirection();
        glm::vec3 right = m_pCamera->GetRight();
        glm::vec3 move = dir * (float)yoffset;
        xoffset = -xoffset;
        move += right * (float)xoffset;
        m_pCamera->GetVPMatrix().Translate(move);
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::SPACE, [&](){
        m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));
        m_pCamera->GetVPMatrix().SetRotation(glm::vec3(0,0,0));
    });

    static bool BtnLeftPressing = false;
    static bool BtnRightPressing = false;
    inputMonitor->AddMousePressedCallback(platform::Mouse::Button::LEFT, [&](){ BtnLeftPressing = true; });
    inputMonitor->AddMouseUpCallback(platform::Mouse::Button::LEFT, [&](){ BtnLeftPressing = false; });
    inputMonitor->AddMousePressedCallback(platform::Mouse::Button::RIGHT, [&](){ BtnRightPressing = true; });
    inputMonitor->AddMouseUpCallback(platform::Mouse::Button::RIGHT, [&](){ BtnRightPressing = false; });

    inputMonitor->AddCursorPositionCallback([&](const glm::vec2& lastPos, const glm::vec2& newPos){
        glm::vec2 offset = newPos - lastPos;
        offset.x = (offset.x - (int)offset.x >=0.5f) ? ((int)offset.x + 1) : (int)offset.x;
        offset.y = (offset.y - (int)offset.y >=0.5f) ? ((int)offset.y + 1) : (int)offset.y;
        if (BtnLeftPressing)
        {
            m_pCamera->GetVPMatrix().Rotate(glm::vec3(-offset.y / 10,0,0));
            m_pCamera->GetVPMatrix().Rotate(glm::vec3(0,-offset.x,0));
        }
        else if (BtnRightPressing)
        {
            glm::vec3 dir = m_pCamera->GetDirection();
            glm::vec3 right = m_pCamera->GetRight();
            glm::vec3 move = dir * -offset.y / 10.f;
            move += right * offset.x / 10.f;
            m_pCamera->GetVPMatrix().Translate(move);
        }
    });
}

void SimpleModelRenderer::prepareRenderpass()
{
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    vk::SampleCountFlagBits sampleCount = m_pDevice->GetVulkanPhysicalDevice()->GetSampleCount();
    m_pRenderPass = std::make_shared<RHI::VulkanRenderPass>(m_pDevice.get(), colorFormat, depthForamt, sampleCount);
}

void SimpleModelRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get())
                            .SetVulkanRenderPass(m_pRenderPass)
                            .SetshaderSet(shaderSet)
                            .SetVulkanPipelineLayout(m_pPipelineLayout)
                            .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("default", std::move(pipeline));
}

void SimpleModelRenderer::prepareFrameBuffer()
{
    m_pDevice->CreateSwapchainFramebuffer(m_pRenderPass.get());
}


void SimpleModelRenderer::updateUniformBuf(uint32_t currentFrameIdx)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    static glm::vec3 lightPos(1, 1, -2);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    auto extent = m_pDevice->GetSwapchainExtent();

    // auto& modelTransformation = m_pModel->GetTransformation();
    // modelTransformation.Rotate(glm::vec3(0, time*90, 0));
    RHI::UniformBufferObject ubo{};
    ubo.model = m_pModel->GetTransformation().GetMatrix();
    ubo.view = m_pCamera->GetViewMatrix();
    ubo.proj = m_pCamera->GetProjMatrix();

    ubo.camPos = m_pCamera->GetPosition();
    ubo.lightPos = lightPos;
    m_pUniformBuffers[currentFrameIdx]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
}