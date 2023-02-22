#include "SimpleModelRenderer.h"
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
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
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

    glm::vec3 camPos(2.0f, 2.0f, 2.0f);
    glm::vec3 lightPos = camPos;

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    auto extent = m_pDevice->GetSwapchainExtent();
    RHI::UniformBufferObject ubo{};
    ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f,0.1f,0.1f));
    ubo.model *= glm::translate(glm::mat4(1.0f), glm::vec3(4, 4,-4));
    ubo.model *= glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.model *= glm::rotate(glm::mat4(1.0f), glm::radians(135.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.model *= glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));    ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    ubo.camPos = camPos;
    ubo.lightPos = lightPos;
    m_pUniformBuffers[currentFrameIdx]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
}