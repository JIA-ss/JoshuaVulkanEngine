#include "SimpleModelRenderer.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "vulkan/vulkan_enums.hpp"
#include <Util/Modelutil.h>
#include <Util/Fileutil.h>
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

    m_pVulkanVertexBuffer.reset();
    m_pVulkanVertexIndexBuffer.reset();
    m_pVulkanDescriptorSetLayout.reset();
    m_pVulkanDescriptorSets.reset();
    m_pVulkanShaderSet.reset();
    m_pVulkanRenderPipeline.reset();
}

void SimpleModelRenderer::prepare()
{
    // prepare vertex data
    prepareVertexData();
    // prepare shader set
    // prepare descroptor set
    prepareShader();
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
        m_vkCmds[m_frameIdxInFlight].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pVulkanRenderPipeline->GetVkPipeline());

        m_pVulkanRenderPipeline->GetPVulkanDynamicState()->SetUpCmdBuf(m_vkCmds[m_frameIdxInFlight], m_pVulkanRenderPipeline.get());

        m_vkCmds[m_frameIdxInFlight].bindVertexBuffers(0, *m_pVulkanVertexBuffer->GetPVkBuf(), {0});
        m_vkCmds[m_frameIdxInFlight].bindIndexBuffer(*m_pVulkanVertexIndexBuffer->GetPVkBuf(), 0, vk::IndexType::eUint32);
        m_vkCmds[m_frameIdxInFlight].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pVulkanRenderPipeline->GetVkPipelineLayout(), 0, m_pVulkanDescriptorSets->GetVkDescriptorSet(m_frameIdxInFlight), {});

        std::vector<vk::ClearValue> clears(2);

        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        auto renderpassBeginInfo = vk::RenderPassBeginInfo()
                                .setRenderPass(m_pVulkanRenderPipeline->GetVkRenderPass())
                                .setClearValues(clears)
                                .setRenderArea(vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent})
                                .setFramebuffer(m_pDevice->GetSwapchainFramebuffer(m_imageIdx)); 
        m_vkCmds[m_frameIdxInFlight].beginRenderPass(renderpassBeginInfo , {});
        {
            // m_vkCmds[m_frameIdxInFlight].draw(m_vertices.size(), 1, 0, 0);
            m_vkCmds[m_frameIdxInFlight].drawIndexed(m_indices.size(), 1, 0,0,0);
        }
        m_vkCmds[m_frameIdxInFlight].endRenderPass();
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


void SimpleModelRenderer::prepareVertexData()
{
    auto model = Util::Model::TinyObj(Util::File::getResourcePath() / "Model/viking_room.obj");
    m_vertices = *model.GetPVertices();
    m_indices = *model.GetPIndices();

    m_pVulkanVertexBuffer = RHI::VulkanVertexBuffer::Create(m_pDevice.get(), m_vertices, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_pVulkanVertexBuffer->CopyDataToGPU(m_vkCmds[0], m_pDevice->GetVkGraphicQueue(), m_vertices.size() * sizeof(m_vertices[0]));
    m_vkCmds[0].reset();

    m_pVulkanVertexIndexBuffer = RHI::VulkanVertexIndexBuffer::Create(m_pDevice.get(), m_indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_pVulkanVertexIndexBuffer->CopyDataToGPU(m_vkCmds[0], m_pDevice->GetVkGraphicQueue(), m_indices.size() * sizeof(m_indices[0]));
    m_vkCmds[0].reset();
}

void SimpleModelRenderer::prepareShader()
{

    // descriptor set layout
    m_pVulkanDescriptorSetLayout.reset(new RHI::VulkanDescriptorSetLayout(m_pDevice.get()));
    std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
    bindings[0]
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            ;
    bindings[1]
            .setBinding(1)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ;
    m_pVulkanDescriptorSetLayout->AddBindings(bindings);
    m_pVulkanDescriptorSetLayout->Finish();

    // descriptor set
    std::vector<std::unique_ptr<RHI::VulkanBuffer>> uniformBuffers(MAX_FRAMES_IN_FLIGHT);
    std::vector<std::unique_ptr<RHI::VulkanImageSampler>> images(MAX_FRAMES_IN_FLIGHT);
    auto imageRawData = Util::Texture::RawData::Load(Util::File::getResourcePath() / "Texture/viking_room.png", Util::Texture::RawData::Format::eRgbAlpha);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uniformBuffers[i].reset(
            new RHI::VulkanBuffer(
                    m_pDevice.get(), sizeof(RHI::UniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                )
        );
        RHI::VulkanImageSampler::Config imageSamplerConfig;
        RHI::VulkanImageResource::Config imageResourceConfig;
        imageResourceConfig.extent = vk::Extent3D{(uint32_t)imageRawData->GetWidth(), (uint32_t)imageRawData->GetHeight(), 1};
        images[i].reset(
            new RHI::VulkanImageSampler(
                m_pDevice.get(),
                imageRawData,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                imageSamplerConfig,
                imageResourceConfig
                )
        );
    }
    m_pVulkanDescriptorSets.reset(new RHI::VulkanDescriptorSets(m_pDevice.get(), m_pVulkanDescriptorSetLayout.get(), std::move(uniformBuffers), std::move(images)));


    // shader
    m_pVulkanShaderSet.reset(new RHI::VulkanShaderSet(m_pDevice.get()));
    m_pVulkanShaderSet->AddShader(Util::File::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    m_pVulkanShaderSet->AddShader(Util::File::getResourcePath() / "Shader\\GLSL\\SPIR-V\\shader.frag.spv", vk::ShaderStageFlagBits::eFragment);

}

void SimpleModelRenderer::prepareRenderpass()
{
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    vk::SampleCountFlagBits sampleCount = m_pDevice->GetVulkanPhysicalDevice()->GetSampleCount();
    m_pRenderPass.reset(new RHI::VulkanRenderPass(m_pDevice.get(), colorFormat, depthForamt, sampleCount));
}

void SimpleModelRenderer::preparePipeline()
{
    m_pVulkanRenderPipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get())
                        .SetshaderSet(m_pVulkanShaderSet)
                        .SetdescriptorSetLayout(m_pVulkanDescriptorSetLayout)
                        .SetVulkanRenderPass(m_pRenderPass)
                        .build();
}

void SimpleModelRenderer::prepareFrameBuffer()
{
    m_pDevice->CreateSwapchainFramebuffer(m_pRenderPass.get());
}


void SimpleModelRenderer::updateUniformBuf(uint32_t currentFrameIdx)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    auto extent = m_pDevice->GetSwapchainExtent();
    RHI::UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));    ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    m_pVulkanDescriptorSets->GetPWriteUniformBuffer(currentFrameIdx)->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
}