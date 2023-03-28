#include "OITRenderer.h"
#include "Runtime/Render/OIT/LinkedList/LinkedListColorPass.h"
#include "Runtime/Render/OIT/LinkedList/LinkedListGeometryPass.h"
#include "Runtime/Render/PostPass/PresentPostPass.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDepthStencilState.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>

using namespace Render;

RHI::VulkanPhysicalDevice::Config OITRenderer::enableDeviceAtomicFeature(const RHI::VulkanPhysicalDevice::Config& physicalConfig)
{
    RHI::VulkanPhysicalDevice::Config validatedConfig = physicalConfig;
    if (!validatedConfig.requiredFeatures.has_value())
    {
        validatedConfig.requiredFeatures = vk::PhysicalDeviceFeatures().setFragmentStoresAndAtomics(VK_TRUE);
    }
    else
    {
        validatedConfig.requiredFeatures->setFragmentStoresAndAtomics(VK_TRUE);
    }
    return validatedConfig;
}

OITRenderer::OITRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, enableDeviceAtomicFeature(physicalConfig))
{

}

void OITRenderer::prepare()
{
    prepareLayout();
    preparePresentFramebufferAttachments();
    prepareRenderpass();
    preparePipeline();

    // prepare camera
    prepareCamera();
    // prepare Light
    prepareLight();
    // prepare descriptor layout
    prepareModel();
    // prepare callback
    prepareInputCallback();

    prepareLinkedListPass();
    preparePresentFramebuffer();
}

void OITRenderer::render()
{
    ZoneScopedN("DeferredRenderer::render");
    // wait for fence
    if (
        m_pDevice->GetVkDevice().waitForFences(m_vkFences[m_frameIdxInFlight], true, std::numeric_limits<uint64_t>::max())
        !=
        vk::Result::eSuccess
    )
    {
        throw std::runtime_error("wait for inflight fence failed");
    }
    uint32_t m_imageIdx = 0;
    // acquire image
    vk::Result acquireImageResult;
    try
    {
        auto res = m_pDevice->GetVkDevice().acquireNextImageKHR(m_pDevice->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvaliables[m_frameIdxInFlight], nullptr);
        acquireImageResult = res.result;
        m_imageIdx = res.value;
        if (res.result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapchain();
            return;
        }
    }
    catch(vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (m_frameBufferSizeChanged || acquireImageResult == vk::Result::eErrorOutOfDateKHR)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
        return;
    }

    if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("acquire next image failed");
    }

    m_pModel->UpdateModelUniformBuffer();
    m_pCamera->UpdateUniformBuffer();
    m_pLight->UpdateLightUBO();


    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[m_frameIdxInFlight].begin(beginInfo);
    {
        TracyVkCollect(m_tracyVkCtx[m_frameIdxInFlight], m_vkCmds[m_frameIdxInFlight]);
        TracyVkZone(m_tracyVkCtx[m_frameIdxInFlight], m_vkCmds[m_frameIdxInFlight], "deferred");

        {
            // opaque pass
            std::vector<vk::ClearValue> clearValues
            {
                vk::ClearColorValue { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } },
                vk::ClearDepthStencilValue { 1.0f, 0 }
            };
            vk::Rect2D renderArea { {}, { m_pDevice->GetSwapchainExtent().width, m_pDevice->GetSwapchainExtent().height}};
            m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clearValues, renderArea, m_opaquePass.framebuffer->GetVkFramebuffer());
            {
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)renderArea.extent.width, (float)renderArea.extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,renderArea);
                std::vector<vk::DescriptorSet> tobinding;
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "skybox");
                m_pSkyBox->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);
            }
            m_pRenderPass->End(m_vkCmds[m_frameIdxInFlight]);


            vk::ImageMemoryBarrier barrier;
            barrier.setImage(m_opaquePass.colorAttachmentSampler->GetPImageResource()->GetVkImage())
                    .setSubresourceRange(vk::ImageSubresourceRange().setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1).setLevelCount(1))
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                    .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    ;
            m_vkCmds[m_frameIdxInFlight].pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
        }

        {
            // geometry pass
            ZoneScopedN("DeferredRenderer::render::geometry pass");
            m_linkedlistPass.geometryPass->Render(m_vkCmds[m_frameIdxInFlight], { m_pModel.get() });
        }

        // m_opaquePass.colorAttachmentSampler->GetPImageResource()->TransitionImageLayout(m_vkCmds[m_frameIdxInFlight], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        {
            // color pass
            std::vector<vk::DescriptorSet> tobinding(3);
            tobinding[1] = m_opaquePass.descriptorSet->GetVkDescriptorSet(0);
            auto desc = m_linkedlistPass.geometryPass->GetDescriptorSets();
            tobinding[2] = desc->GetVkDescriptorSet(0);
            m_linkedlistPass.colorPass->Render(m_vkCmds[m_frameIdxInFlight], tobinding, m_pDevice->GetVulkanPresentFramebuffer(m_imageIdx)->GetVkFramebuffer());

            // vk::MemoryBarrier enterBarrier;
            // enterBarrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            //     .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead);
            // m_vkCmds[m_frameIdxInFlight].pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
            // vk::DependencyFlags(), enterBarrier, nullptr, nullptr);

        }
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

void OITRenderer::prepareLayout()
{
    m_pPipelineLayout.reset(new RHI::VulkanPipelineLayout(
        m_pDevice.get(),
        {m_pDevice->GetDescLayoutPresets().UBO, m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER}));
}

void OITRenderer::prepareRenderpass()
{
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();

    auto sampleCount = m_pPhysicalDevice->GetSampleCount();

    RHI::VulkanImageResource::Config imageConfig;
    imageConfig.extent = vk::Extent3D{ m_pDevice->GetSwapchainExtent(), 1};
    imageConfig.sampleCount = sampleCount;

    // depth
    {
        imageConfig.format = m_pPhysicalDevice->QuerySupportedDepthFormat();
        imageConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
        if (m_pPhysicalDevice->HasStencilComponent(imageConfig.format))
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        }
        else
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        }
        RHI::VulkanImageSampler::Config samplerConfig;
        samplerConfig.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        m_opaquePass.depthAttachmentSampler.reset(new RHI::VulkanImageSampler(m_pDevice.get(), nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig, imageConfig));
        m_opaquePass.depthAttachmentSampler->GetPImageResource()->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    }

    // color
    imageConfig.format = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    imageConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    imageConfig.subresourceRange
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    ;
    RHI::VulkanImageSampler::Config samplerConfig;
    m_opaquePass.colorAttachmentSampler.reset(new RHI::VulkanImageSampler(m_pDevice.get(), nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig, imageConfig));
    m_opaquePass.colorAttachmentSampler->GetPImageResource()->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);



    m_opaquePass.attachments.resize(2);
    // color
    m_opaquePass.attachments[0].type = RHI::VulkanFramebuffer::kColor;
    m_opaquePass.attachments[0].samples = sampleCount;
    m_opaquePass.attachments[0].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    m_opaquePass.attachments[0].resourceFinalLayout = vk::ImageLayout::eColorAttachmentOptimal;
    m_opaquePass.attachments[0].resource = m_opaquePass.colorAttachmentSampler->GetPImageResource()->GetNative();
    m_opaquePass.attachments[0].resourceFormat = colorFormat;



    // depth
    m_opaquePass.attachments[1].type = RHI::VulkanFramebuffer::kDepthStencil;
    m_opaquePass.attachments[1].samples = sampleCount;
    m_opaquePass.attachments[1].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_opaquePass.attachments[1].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_opaquePass.attachments[1].resource = m_opaquePass.depthAttachmentSampler->GetPImageResource()->GetNative();
    m_opaquePass.attachments[1].resourceFormat = depthForamt;


    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice.get())
                        .SetAttachments(m_opaquePass.attachments)
                        .AddSubpassDependency
                        (
                            vk::SubpassDependency()
                                    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                                    .setDstSubpass(0)
                                    .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                                    .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                        )
                        // .AddSubpassDependency
                        // (
                        //     vk::SubpassDependency()
                        //         .setSrcSubpass(0)
                        //         .setDstSubpass(2)
                        //         .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        //         .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                        //         .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eShaderWrite)
                        //         .setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead)
                        //         .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
                        // )
                        .buildUnique();


    m_opaquePass.framebuffer.reset(new RHI::VulkanFramebuffer(m_pDevice.get(), m_pRenderPass.get(), imageConfig.extent.width, imageConfig.extent.height, 1, m_opaquePass.attachments));


    m_opaquePass.descriptorPool.reset(new RHI::VulkanDescriptorPool(m_pDevice.get(),
    {vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1}}, 1));
    m_opaquePass.descriptorSet = m_opaquePass.descriptorPool->AllocSamplerDescriptorSet(
        m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER.get(),
        {m_opaquePass.colorAttachmentSampler.get()}, std::vector<uint32_t> {1});
}

void OITRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> skyboxShader = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    skyboxShader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
    skyboxShader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);
    
    RHI::VulkanDepthStencilState::Config depthTestConfig;
    depthTestConfig.DepthCompareOp = vk::CompareOp::eLessOrEqual;
    auto depthStencilState = std::make_shared<RHI::VulkanDepthStencilState>(depthTestConfig);
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                        .SetVulkanPipelineLayout(m_pPipelineLayout)
                        .SetshaderSet(skyboxShader)
                        .SetVulkanDepthStencilState(depthStencilState)
                        .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("skybox", std::move(pipeline));
}

void OITRenderer::preparePresentFramebuffer()
{
    ZoneScopedN("OITRenderer::preparePresentFramebuffer");
    m_pDevice->CreateVulkanPresentFramebuffer(
        m_linkedlistPass.colorPass->GetPVulkanRenderPass(),
        m_pDevice->GetSwapchainExtent().width,
        m_pDevice->GetSwapchainExtent().height,
        1,
        m_VulkanPresentFramebufferAttachments,
        getPresentImageAttachmentId()
    );
}

void OITRenderer::prepareCamera()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 500.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));

    m_pCamera->InitUniformBuffer(m_pDevice.get());
}
void OITRenderer::prepareLight()
{
    m_pLight.reset(new Lights(m_pDevice.get(), 1));
    m_pLight->GetLightTransformation().SetPosition(glm::vec3(1, 1, -2));
}
void OITRenderer::prepareModel()
{
    m_pSkyBox = RHI::ModelPresets::CreateSkyboxModel(m_pDevice.get(), m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER.get());
    m_pModel.reset(new RHI::Model(m_pDevice.get(), Util::File::getResourcePath() / "Model/nanosuit/nanosuit.obj", m_pSet1SamplerSetLayout.lock().get()));
    auto& transformation = m_pModel->GetTransformation();
    transformation.SetPosition(glm::vec3(0.0f, 0.f, 0.f));
    transformation.SetRotation(glm::vec3(0,90,0));
    transformation.SetScale(glm::vec3(1.f));

    m_pModel->SetColor(glm::vec4(1,1,1,0.5));
    m_pSkyBox->SetColor(glm::vec4(1));

    std::vector<RHI::Model::UBOLayoutInfo> uboInfos;
    auto camUbo = m_pCamera->GetUboInfo();
    auto lightUbo = m_pLight->GetUboInfo();
    uboInfos.push_back(camUbo);
    uboInfos.push_back(lightUbo);
    m_pModel->InitUniformDescriptorSets(uboInfos);
    m_pSkyBox->InitUniformDescriptorSets(uboInfos);
}

void OITRenderer::prepareInputCallback()
{
    auto inputMonitor = m_pPhysicalDevice->GetPWindow()->GetInputMonitor();
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::W, [&](){
        glm::vec3 dir = m_pCamera->GetDirection();
        m_pCamera->GetVPMatrix().Translate(dir * 0.1f);
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::S, [&](){
        glm::vec3 dir = m_pCamera->GetDirection();
        m_pCamera->GetVPMatrix().Translate(dir * -0.1f);
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::A, [&](){
        glm::vec3 right = m_pCamera->GetRight();
        m_pCamera->GetVPMatrix().Translate(right * -0.1f);
    });


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::D, [&](){
        glm::vec3 right = m_pCamera->GetRight();
        m_pCamera->GetVPMatrix().Translate(right * 0.1f);
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
    inputMonitor->AddMousePressedCallback(platform::Mouse::MIDDLE, [&](){
        m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));
        m_pCamera->GetVPMatrix().SetRotation(glm::vec3(0,0,0));
    });

    inputMonitor->AddCursorPositionCallback([&](const glm::vec2& lastPos, const glm::vec2& newPos){
        glm::vec2 offset = newPos - lastPos;
        offset.x = (offset.x - (int)offset.x >=0.5f) ? ((int)offset.x + 1) : (int)offset.x;
        offset.y = (offset.y - (int)offset.y >=0.5f) ? ((int)offset.y + 1) : (int)offset.y;
        if (BtnLeftPressing)
        {
            m_pCamera->GetVPMatrix().Rotate(glm::vec3(-offset.y / 30,0,0));
            m_pCamera->GetVPMatrix().Rotate(glm::vec3(0,-offset.x / 30,0));
        }
        else if (BtnRightPressing)
        {
            glm::vec3 dir = m_pCamera->GetDirection();
            glm::vec3 move = dir * -offset.y / 100.f;
            m_pCamera->GetVPMatrix().Translate(move);
        }
    });
}
void OITRenderer::prepareLinkedListPass()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    m_linkedlistPass.geometryPass =
        std::make_unique<LinkedListGeometryPass>(
            m_pDevice.get(),
            m_pCamera.get(),
            extent.width, extent.height,
            m_opaquePass.depthAttachmentSampler->GetPImageResource()
        );

    auto colorShader = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    colorShader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/linkedlist-color.vert.spv", vk::ShaderStageFlagBits::eVertex);
    colorShader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/linkedlist-color.frag.spv", vk::ShaderStageFlagBits::eFragment);

    m_linkedlistPass.colorPass =
        std::make_unique<LinkedListColorPass>(
            m_pDevice.get(), m_pCamera.get(), m_pLight.get(),
            extent.width, extent.height,
            colorShader,
            m_VulkanPresentFramebufferAttachments,
            m_linkedlistPass.geometryPass->GetLinkedListDescriptorSetLayout());
    m_linkedlistPass.colorPass->Prepare();
}