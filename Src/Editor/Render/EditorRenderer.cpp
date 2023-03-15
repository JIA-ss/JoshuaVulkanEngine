#include "EditorRenderer.h"
#include "Editor/Editor.h"
#include "Editor/Window/EditorWindow.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <memory>

EDITOR_NAMESPACE_USING

EditorRenderer::EditorRenderer(
    Render::RendererBase* runtime_renderer,
    const RHI::VulkanInstance::Config& instanceConfig,
    const RHI::VulkanPhysicalDevice::Config& physicalConfig)
    : RendererBase(instanceConfig, physicalConfig)
    , m_runtimeRenderer(runtime_renderer)
{

}

EditorRenderer::~EditorRenderer()
{
    //please implement deconstruct function
    // assert(false);
}

void EditorRenderer::prepare()
{
    prepareLayout();
    preparePresentFramebufferAttachments();
    prepareRenderpass();
    preparePresentFramebuffer();
    preparePipeline();

    // prepare camera
    prepareCamera();
    // prepare light
    prepareLight();
    // prepare descriptor layout
    prepareModel();
    // prepare callback
    prepareInputCallback();
}

void EditorRenderer::render()
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
    vk::Result acquireImageResult;
    uint32_t m_imageIdx;
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


    m_pCamera->UpdateUniformBuffer(m_frameIdxInFlight);
    m_pSceneCameraFrustumModel->GetTransformation().SetPosition(m_sceneCamera->GetPosition())
                .SetRotation(m_sceneCamera->GetVPMatrix().GetRotation());

    for(int i = 0; i < m_pSceneLightFrustumModels.size(); i++)
    {
        m_pSceneLightFrustumModels[i]->GetTransformation().SetPosition(m_sceneLights->GetLightTransformation(i).GetPosition())
                .SetRotation(m_sceneLights->GetLightTransformation(i).GetRotation());
    }

    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[m_frameIdxInFlight].begin(beginInfo);
    {
        std::vector<vk::DescriptorSet> tobinding;

        std::vector<vk::ClearValue> clears(2);
        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, m_pDevice->GetVulkanPresentFramebuffer(m_imageIdx)->GetVkFramebuffer());
        {
            {
                // draw scene object
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "default");
                auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
                vk::Rect2D rect{{0,0},extent};
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);

                for (auto& modelView : m_sceneModels)
                {
                    modelView->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);
                }
            }

            {
                // draw scene camera and light frustum
                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "frustum");
                m_pSceneCameraFrustumModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);
                for (auto& lightFrustum : m_pSceneLightFrustumModels)
                {
                    lightFrustum->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);
                }
            }

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

void EditorRenderer::prepareLayout()
{
    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice.get(),
            {m_pSet0UniformSetLayout.lock(), m_pSet1SamplerSetLayout.lock(), m_pSet2ShadowmapSamplerLayout.lock()}
            , {}
            )
        );
}

void EditorRenderer::preparePresentFramebufferAttachments()
{
    vk::Format colorFormat = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().format.format;
    vk::Format depthForamt = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();

    RHI::VulkanImageResource::Config imageConfig;
    imageConfig.extent = vk::Extent3D{ m_pDevice->GetSwapchainExtent(), 1};
    imageConfig.sampleCount = vk::SampleCountFlagBits::e1;

    // present
    {
    }

    // depth
    {
        imageConfig.format = m_pPhysicalDevice->QuerySupportedDepthFormat();
        imageConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        if (m_pPhysicalDevice->HasStencilComponent(imageConfig.format))
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
        }
        else
        {
            imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
        }
        m_attachmentResources.depthVulkanImageResource.reset(new RHI::VulkanImageResource(m_pDevice.get(), vk::MemoryPropertyFlagBits::eDeviceLocal, imageConfig));
    }


    m_VulkanPresentFramebufferAttachments.resize( 2);
    // present
    m_VulkanPresentFramebufferAttachments[0].type = RHI::VulkanFramebuffer::kColor;
    m_VulkanPresentFramebufferAttachments[0].resourceFinalLayout = vk::ImageLayout::ePresentSrcKHR;
    m_VulkanPresentFramebufferAttachments[0].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[0].resourceFormat = colorFormat;

    // depth
    m_VulkanPresentFramebufferAttachments[1].type = RHI::VulkanFramebuffer::kDepthStencil;
    m_VulkanPresentFramebufferAttachments[1].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[1].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    m_VulkanPresentFramebufferAttachments[1].resource = m_attachmentResources.depthVulkanImageResource->GetNative();
    m_VulkanPresentFramebufferAttachments[1].resourceFormat = depthForamt;
}

void EditorRenderer::prepareCamera()
{

    m_sceneCamera = m_runtimeRenderer->GetCamera();

    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 5000.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));

    m_pCamera->InitUniformBuffer(m_pDevice.get());
}

void EditorRenderer::prepareLight()
{
    m_pLight.reset(new Render::Lights(m_pDevice.get(), 1));
    m_pLight->GetLightTransformation().SetPosition(glm::vec3(1, 1, -2));

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_pLight->UpdateLightUBO(i);
    }

    m_sceneLights = m_runtimeRenderer->GetLights();
}

void EditorRenderer::prepareModel()
{
    std::array<std::vector<RHI::Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT> uboInfos;
    auto camUbo = m_pCamera->GetUboInfo();
    auto lightUbo = m_pLight->GetUboInfo();
    for (int frameId = 0; frameId < uboInfos.size(); frameId++)
    {
        uboInfos[frameId].push_back(camUbo[frameId]);
        uboInfos[frameId].push_back(lightUbo[frameId]);
    }

    auto models = m_runtimeRenderer->GetModels();
    for (auto& model : models)
    {
        m_sceneModels.push_back(std::make_shared<RHI::ModelView>(model, m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get()));
        m_sceneModels.back()->InitUniformDescriptorSets(uboInfos);
    }

    m_pSceneCameraFrustumModel = RHI::ModelPresets::CreateFrustumModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get(), m_sceneCamera->GetVPMatrix());
    m_pSceneCameraFrustumModel->SetColor(glm::vec4(0.5f,1.0f,0,1));
    m_pSceneCameraFrustumModel->InitUniformDescriptorSets(uboInfos);
    if (m_sceneLights)
    {
        m_pSceneLightFrustumModels.resize(m_sceneLights->GetLightNum());
        for (int i = 0; i < m_sceneLights->GetLightNum(); i++)
        {
            m_pSceneLightFrustumModels[i] = RHI::ModelPresets::CreateFrustumModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get(), m_sceneLights->GetLightTransformation(i));
            m_pSceneLightFrustumModels[i]->InitUniformDescriptorSets(uboInfos);
            m_pSceneLightFrustumModels[i]->SetColor(glm::vec4(1,1,0,1));
        }
    }
}

void EditorRenderer::prepareInputCallback()
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


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::SPACE, [&](){
        std::cout << "Cam Pos: " << glm::to_string(m_pCamera->GetPosition()) << std::endl;
        std::cout << "Cam Rot: " << glm::to_string(m_pCamera->GetVPMatrix().GetRotation()) << std::endl;
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
            {
                m_pCamera->GetVPMatrix().Rotate(glm::vec3(-offset.y / 30,0,0))
                                        .Rotate(glm::vec3(0,-offset.x / 30,0));
            }
        }
        else if (BtnRightPressing)
        {
            {
                glm::vec3 dir = m_pCamera->GetDirection();
                glm::vec3 move = dir * -offset.y / 100.f;
                m_pCamera->GetVPMatrix().Translate(move);
            }
        }
    });
}

void EditorRenderer::prepareRenderpass()
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice.get())
                            .SetAttachments(m_VulkanPresentFramebufferAttachments)
                            .buildUnique();
}

void EditorRenderer::preparePresentFramebuffer()
{
    m_pDevice->CreateVulkanPresentFramebuffer(
        m_pRenderPass.get(),
        m_pDevice->GetSwapchainExtent().width,
        m_pDevice->GetSwapchainExtent().height,
        1,
        m_VulkanPresentFramebufferAttachments,
        getPresentImageAttachmentId()
    );
}

void EditorRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shader.frag.spv", vk::ShaderStageFlagBits::eFragment);
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                            .SetshaderSet(shaderSet)
                            .SetVulkanPipelineLayout(m_pPipelineLayout)
                            .SetVulkanMultisampleState(std::make_shared<RHI::VulkanMultisampleState>(vk::SampleCountFlagBits::e1))
                            .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("default", std::move(pipeline));


    std::shared_ptr<RHI::VulkanRasterizationState> rasterization = RHI::VulkanRasterizationStateBuilder()
                                                                    .SetPolygonMode(vk::PolygonMode::eLine)
                                                                    .SetLineWidth(3.0f)
                                                                    .SetCullMode(vk::CullModeFlagBits::eNone)
                                                                    .build();
    auto dynamicstate = std::make_shared<RHI::VulkanDynamicState>(std::vector<vk::DynamicState>{
        vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth
    });
    auto rawLinePipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                        .SetVulkanRasterizationState(rasterization)
                        .SetVulkanPipelineLayout(m_pPipelineLayout)
                        .SetshaderSet(shaderSet)
                        .SetVulkanDynamicState(dynamicstate)
                        .SetVulkanMultisampleState(std::make_shared<RHI::VulkanMultisampleState>(vk::SampleCountFlagBits::e1))
                        .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("frustum", std::move(rawLinePipeline));
}