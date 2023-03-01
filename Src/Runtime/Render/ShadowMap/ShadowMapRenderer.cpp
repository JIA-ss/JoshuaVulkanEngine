#include "ShadowMapRenderer.h"
#include "Runtime/Render/ShadowMap/ShadowMapRenderer.h"
#include "Runtime/VulkanRHI/Graphic/Mesh.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Fileutil.h"
#include "Util/Mathutil.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

using namespace Render;

ShadowMapRenderer::ShadowMapRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, physicalConfig)
{

}

ShadowMapRenderer::~ShadowMapRenderer()
{
    m_pDevice->GetVkDevice().waitIdle();
    m_pModel.reset();
    m_pCamera.reset();

}

void ShadowMapRenderer::prepare()
{
    // prepare camera
    prepareCamera();

    // prepare light
    prepareLights();
    prepareShadowMapPass();

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

void ShadowMapRenderer::render()
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

    m_pLights->UpdateLightUBO(m_imageIdx);

    if (m_renderFromLight)
    {
        RHI::CameraUniformBufferObject ubo;
        ubo.camPos = glm::vec4(m_pLights->GetLightTransformation(0).GetPosition(),1.0f);
        ubo.view = m_pLights->GetLightTransformation(0).GetViewMatrix();
        ubo.proj = m_pLights->GetLightTransformation(0).GetProjMatrix();
        m_pCamera->SetUniformBufferObject(m_imageIdx, &ubo);
    }
    else
    {
        m_pCamera->UpdateUniformBuffer(m_imageIdx);
    }

    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[m_frameIdxInFlight].begin(beginInfo);
    {
        // shadow map pass
        {
            updateShadowMapMVPUniformBuf(m_frameIdxInFlight);
            m_pShadwomapPass->Render(m_vkCmds[m_frameIdxInFlight], {m_pModel.get(), m_pCubeModel.get()}, m_frameIdxInFlight);
        }

        // scene pass
        {
            std::vector<vk::DescriptorSet> tobinding;
            m_pShadwomapPass->FillDepthSamplerToBindedDescriptorSetsVector(tobinding, m_pPipelineLayout.get(), m_frameIdxInFlight);
            std::vector<vk::ClearValue> clears(2);
            clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
            clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
            m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, m_pDevice->GetSwapchainFramebuffer(m_imageIdx));
            {
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "default");
                auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
                vk::Rect2D rect{{0,0},extent};
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);

                m_pCubeModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);
                m_pModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);

                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "[debug]show shadowmap texture");
                m_pDebugShadowMapQuadModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);
            }
            m_pRenderPass->End(m_vkCmds[m_frameIdxInFlight]);
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
void ShadowMapRenderer::prepareLights()
{
    std::vector<Util::Math::VPMatrix> lightTransformations =
    {
        Util::Math::VPMatrix{
            45.f, 1.f, 5.f, 70.f,
            glm::vec3(-35.833206, 4.366684, 0.000000),
            glm::vec3(0.675352, 19.794977, 14.088803)
        }
    };
    m_pLights.reset(new Lights(m_pDevice.get(), lightTransformations, true));

}

void ShadowMapRenderer::prepareModel()
{

    std::array<std::vector<RHI::Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT> uboInfos;
    auto camUbo = m_pCamera->GetUboInfo();
    auto lightUbo = m_pLights->GetUboInfo();
    for (int frameId = 0; frameId < uboInfos.size(); frameId++)
    {
        uboInfos[frameId].push_back(camUbo[frameId]);
        uboInfos[frameId].push_back(lightUbo[frameId]);
    }

    m_pModel.reset(new RHI::Model(m_pDevice.get(), Util::File::getResourcePath() / "Model/nanosuit/nanosuit.obj", m_pSet1SamplerSetLayout.lock().get()));
    auto& transformation = m_pModel->GetTransformation();
    transformation.SetPosition(glm::vec3(0.0f, 0.f, 0.f));
    transformation.SetRotation(glm::vec3(0,0,0));
    m_pModel->InitUniformDescriptorSets(uboInfos);
    m_pShadwomapPass->InitModelShadowDescriptor(m_pModel.get());


    m_pCubeModel = RHI::Model::CreateCubeModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get());
    m_pCubeModel->GetTransformation().SetScale(glm::vec3(100,1,100));
    m_pCubeModel->GetTransformation().SetPosition(glm::vec3(-50.f, -1.f, 50.f));
    m_pCubeModel->InitUniformDescriptorSets(uboInfos);
    m_pShadwomapPass->InitModelShadowDescriptor(m_pCubeModel.get());

    m_pDebugShadowMapQuadModel = RHI::Model::CreatePlaneModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get());
    m_pDebugShadowMapQuadModel->GetTransformation().SetScale(glm::vec3(0.33f, 0.33f, 0.0f));
    m_pDebugShadowMapQuadModel->GetTransformation().SetPosition(glm::vec3(-1.f, -1.f, 0.0f));
    m_pDebugShadowMapQuadModel->InitUniformDescriptorSets(uboInfos);
    m_pShadwomapPass->InitModelShadowDescriptor(m_pDebugShadowMapQuadModel.get());

}

void ShadowMapRenderer::prepareCamera()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 100.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(8.080724, 18.735529, 16.494471));
    m_pCamera->GetVPMatrix().SetRotation(glm::vec3(-27.466455, 18.066822, 0.000000));

    m_pCamera->InitUniformBuffer(m_pDevice.get());
}

void ShadowMapRenderer::prepareInputCallback()
{
    auto inputMonitor = m_pPhysicalDevice->GetPWindow()->GetInputMonitor();

    inputMonitor->AddScrollCallback([&](double xoffset, double yoffset){
        glm::vec3 dir = m_pCamera->GetDirection();
        glm::vec3 right = m_pCamera->GetRight();
        glm::vec3 move = dir * (float)yoffset;
        xoffset = -xoffset;
        move += right * (float)xoffset;
        m_pCamera->GetVPMatrix().Translate(move);
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::W, [&](){
        auto position = m_pLights->GetLightTransformation(0).GetPosition();
        m_pLights->GetLightTransformation(0).SetPosition(position + glm::vec3(0,-1,0));
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::A, [&](){
        auto position = m_pLights->GetLightTransformation(0).GetPosition();
        m_pLights->GetLightTransformation(0).SetPosition(position + glm::vec3(-1,0,0));
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::S, [&](){
        auto position = m_pLights->GetLightTransformation(0).GetPosition();
        m_pLights->GetLightTransformation(0).SetPosition(position + glm::vec3(0,1,0));
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::D, [&](){
        auto position = m_pLights->GetLightTransformation(0).GetPosition();
        m_pLights->GetLightTransformation(0).SetPosition(position + glm::vec3(1,0,0));
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::TAB, [&](){
        m_renderFromLight = !m_renderFromLight;
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::SPACE, [&](){
        std::cout << "Cam Pos: " << glm::to_string(m_pCamera->GetPosition()) << std::endl;
        std::cout << "Cam Rot: " << glm::to_string(m_pCamera->GetVPMatrix().GetRotation()) << std::endl;

        std::cout << "Light Pos: " << glm::to_string(m_pLights->GetLightTransformation(0).GetPosition()) << std::endl;
        std::cout << "Light Rot: " << glm::to_string(m_pLights->GetLightTransformation(0).GetRotation()) << std::endl;
    });

    static bool BtnLeftPressing = false;
    static bool BtnRightPressing = false;
    static bool LeftShiftPressing = false;

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::LEFT_SHIFT, [&](){
        LeftShiftPressing = true;
    });
    inputMonitor->AddKeyboardUpCallback(platform::Keyboard::Key::LEFT_SHIFT, [&](){
        LeftShiftPressing = false;
    });

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
            if (LeftShiftPressing)
            {
                m_pLights->GetLightTransformation(0).Rotate(glm::vec3(-offset.y / 30,0,0))
                                                            .Rotate(glm::vec3(0,-offset.x / 30,0));
            }
            else
            {
                m_pCamera->GetVPMatrix().Rotate(glm::vec3(-offset.y / 30,0,0))
                                        .Rotate(glm::vec3(0,-offset.x / 30,0));
            }
        }
        else if (BtnRightPressing)
        {
            if (LeftShiftPressing)
            {
                glm::vec3 dir = m_pLights->GetLightTransformation(0).GetFrontDir();
                glm::vec3 move = dir * -offset.y / 100.f;
                m_pLights->GetLightTransformation(0).Translate(move);
            }
            else
            {
                glm::vec3 dir = m_pCamera->GetDirection();
                glm::vec3 move = dir * -offset.y / 100.f;
                m_pCamera->GetVPMatrix().Translate(move);
            }
        }
    });
}

void ShadowMapRenderer::prepareShadowMapPass()
{
    m_pShadwomapPass = m_pLights->GetPShadowPass();
}


void ShadowMapRenderer::prepareRenderpass()
{
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    vk::SampleCountFlagBits sampleCount = m_pDevice->GetVulkanPhysicalDevice()->GetSampleCount();
    m_pRenderPass = std::make_shared<RHI::VulkanRenderPass>(m_pDevice.get(), colorFormat, depthForamt, sampleCount);
}

void ShadowMapRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shadowmap.scene.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shadowmap.scene.frag.spv", vk::ShaderStageFlagBits::eFragment);

    std::shared_ptr<RHI::VulkanRasterizationState> raster = RHI::VulkanRasterizationStateBuilder()
                                                                                    .SetCullMode(vk::CullModeFlagBits::eNone).build();
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get())
                            .SetVulkanRenderPass(m_pRenderPass)
                            .SetshaderSet(shaderSet)
                            .SetVulkanPipelineLayout(m_pPipelineLayout)
                            .SetVulkanRasterizationState(raster)
                            .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("default", std::move(pipeline));


    {
        // [debug]show shadowmap texture pipeline
        std::shared_ptr<RHI::VulkanShaderSet> debugShaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
        debugShaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/debug.quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
        debugShaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/debug.quad.shadow.frag.spv", vk::ShaderStageFlagBits::eFragment);

        RHI::VulkanDepthStencilState::Config depthConfig{};
        depthConfig.DepthCompareOp = vk::CompareOp::eAlways;
        depthConfig.DepthWriteEnable = VK_FALSE;

        std::shared_ptr<RHI::VulkanDepthStencilState> debugDepthState(new RHI::VulkanDepthStencilState(depthConfig));
        pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get())
                    .SetVulkanRenderPass(m_pRenderPass)
                    .SetshaderSet(debugShaderSet)
                    .SetVulkanPipelineLayout(m_pPipelineLayout)
                    .SetVulkanDepthStencilState(debugDepthState)
                    .buildUnique();
        m_pRenderPass->AddGraphicRenderPipeline("[debug]show shadowmap texture", std::move(pipeline));
    }
}

void ShadowMapRenderer::prepareFrameBuffer()
{
    m_pDevice->CreateSwapchainFramebuffer(m_pRenderPass.get());
}


void ShadowMapRenderer::updateShadowMapMVPUniformBuf(uint32_t currentFrameIdx)
{
    for (int i = 0; i < m_pLights->GetLightNum(); i++)
    {
        RHI::CameraUniformBufferObject ubo{};
        ubo.view = m_pLights->GetLightTransformation(i).GetViewMatrix();
        ubo.proj = m_pLights->GetLightTransformation(i).GetProjMatrix();
        ubo.camPos = glm::vec4(m_pLights->GetLightTransformation(i).GetPosition(), 1);

        m_pShadwomapPass->SetShadowPassLightVPUBO(ubo, currentFrameIdx, i);
    }
}
