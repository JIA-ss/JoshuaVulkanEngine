#include "DeferredRenderer.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_enums.hpp"
#include <memory>
#include <stdint.h>
#include <tracy/Tracy.hpp>
#include <random>

using namespace Render;

DeferredRenderer::DeferredRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, physicalConfig)
{

}

DeferredRenderer::~DeferredRenderer()
{
   //please implement deconstruct function
   assert(false);
}

void DeferredRenderer::prepare()
{

    prepareLayout();
    preparePresentFramebufferAttachments();
    prepareRenderpass();
    preparePresentFramebuffer();
    preparePipeline();

    // prepare camera
    prepareCamera();
    // prepare Light
    prepareLight();
    // prepare descriptor layout
    prepareModel();

    // prepare callback
    prepareInputCallback();


    prepareGeometryPrePass();

}

void DeferredRenderer::render()
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

    m_pPlaneModel->UpdateModelUniformBuffer();
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
            // geometry pass
            ZoneScopedN("DeferredRenderer::render::geometry pass");
            m_pGeometryPass->Render(m_vkCmds[m_frameIdxInFlight], {m_pSceneModel.get()});
        }

        {
            // lighting pass
            m_pPipelineLayout->PushConstantT(m_vkCmds[m_frameIdxInFlight], 0, m_pushConstant, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);


            std::vector<vk::ClearValue> clears(2);
            clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
            clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
            if (m_pPhysicalDevice->IsUsingMSAA())
            {
                clears.push_back(clears[0]);
            }

            std::vector<vk::DescriptorSet> tobinding(2);
            tobinding[1] = m_pGeometryPass->GetDescriptorSets()->GetVkDescriptorSet(0);

            m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, m_pDevice->GetVulkanPresentFramebuffer(m_imageIdx)->GetVkFramebuffer());
            {
                ZoneScopedN("DeferredRenderer::render::lighting pass");
                auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
                vk::Rect2D rect{{0,0},extent};
                m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
                m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);

                // m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "skybox");
                // m_pSkyboxModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);

                m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "shading");
                // m_vkCmds[m_frameIdxInFlight].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pPipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});
                m_pPlaneModel->DrawWithNoMaterial(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding);
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

void DeferredRenderer::prepareLayout()
{
    std::map<int, vk::PushConstantRange> pushconstants
    {
        {
            0,
            vk::PushConstantRange
            {
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(m_pushConstant)
            }
        }
    };

    m_pCustomDescriptorSetLayout = m_pDevice->GetDescLayoutPresets().CreateCustomUBO(m_pDevice.get(), vk::ShaderStageFlagBits::eFragment);
    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice.get(),
            {m_pCustomDescriptorSetLayout, m_pSet1SamplerSetLayout.lock(), m_pSet2ShadowmapSamplerLayout.lock()}
            ,pushconstants
            )
        );
}


void DeferredRenderer::prepareRenderpass()
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice.get())
                            .SetAttachments(m_VulkanPresentFramebufferAttachments)
                            .buildUnique();
}

void DeferredRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> shader = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shader->AddShader(Util::File::getResourcePath()/"Shader/GLSL/SPIR-V/defershading.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shader->AddShader(Util::File::getResourcePath()/"Shader/GLSL/SPIR-V/defershading.frag.spv", vk::ShaderStageFlagBits::eFragment);
    m_pRenderPass->AddGraphicRenderPipeline("shading",
        RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
            .SetVulkanPipelineLayout(m_pPipelineLayout)
            .SetshaderSet(shader)
            .buildUnique());
}

void DeferredRenderer::prepareGeometryPrePass()
{
    constexpr const uint32_t geoPassFbDim = 2048;
    m_pGeometryPass.reset(new PrePass(m_pDevice.get(), m_pCamera.get(), geoPassFbDim, geoPassFbDim));
}

void DeferredRenderer::prepareCamera()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 2000.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(-641,237,-16))
                            .SetRotation(glm::vec3(-17.96,-80.76,0));

    m_pCamera->InitUniformBuffer(m_pDevice.get());
}
void DeferredRenderer::prepareModel()
{
    m_pPlaneModel = RHI::ModelPresets::CreatePlaneModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get());
    m_pSceneModel.reset(new RHI::Model(m_pDevice.get(), Util::File::getResourcePath() / "Model/Sponza-master/sponza.obj",  m_pSet1SamplerSetLayout.lock().get()));
    auto camUboInfo = m_pCamera->GetUboInfo();
    auto lightUboInfo = m_pLight->GetUboInfo();
    auto lightLargeUboInfo = m_pLight->GetLargeUboInfo();
    m_pSceneModel->InitUniformDescriptorSets({camUboInfo, lightUboInfo});
    m_pPlaneModel->InitUniformDescriptorSets({camUboInfo, lightLargeUboInfo}, m_pCustomDescriptorSetLayout.get());
}
void DeferredRenderer::prepareLight()
{
    constexpr const int groupSize = 5;
    constexpr const int eachGroupLight = 20;
    m_pLight.reset(new Lights(m_pDevice.get(), groupSize * eachGroupLight));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> color(0.0f, 1.0f);
    float y = 50;

    float lightZPos = -300;
    float zStep = 110;
    for (int group = 0; group < groupSize; group++)
    {
        lightZPos += zStep;

        float lightXPos = -800;
        float xStep = 100;
        for (int light = 0; light < eachGroupLight; light++)
        {
            lightXPos += xStep;

            int idx = group * eachGroupLight + light;
            m_pLight->GetLightTransformation(idx)
                                    .SetFar(300)
                                    .SetPosition(glm::vec3(lightXPos, y, lightZPos))
                                    .SetRotation(glm::vec3(-90,0,0));
            m_pLight->SetLightColor(glm::vec4(color(gen), color(gen), color(gen), 1.0f), idx);
        }
    }
}


void DeferredRenderer::prepareInputCallback()
{
    // m_pushConstant ++, if press Tab
    auto inputMonitor = m_pPhysicalDevice->GetPWindow()->GetInputMonitor();
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::TAB, [&](){
        m_pushConstant.showDebugTarget =
            m_pushConstant.showDebugTarget >= 5
            ? 0
            : m_pushConstant.showDebugTarget + 1;
    });



    static bool BtnLeftPressing = false;
    static bool BtnRightPressing = false;
    static bool LeftShiftPressing = false;


    inputMonitor->AddScrollCallback([&](double xoffset, double yoffset){
        if (LeftShiftPressing)
        {
            glm::vec3 dir = m_pLight->GetLightTransformation().GetFrontDir();
            glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));
            glm::vec3 move = dir * (float)yoffset;
            xoffset = -xoffset;
            move += right * (float)xoffset;

            for (int i = 0; i < m_pLight->GetLightNum(); i++)
            {
                m_pLight->GetLightTransformation(i).Translate(move);
            }
        }
        else
        {
            glm::vec3 dir = m_pCamera->GetDirection();
            glm::vec3 right = m_pCamera->GetRight();
            glm::vec3 move = dir * (float)yoffset;
            xoffset = -xoffset;
            move += right * (float)xoffset;
            m_pCamera->GetVPMatrix().Translate(move);
        }
    });



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
                for (int i = 0; i < m_pLight->GetLightNum(); i++) {
                    m_pLight->GetLightTransformation(i).Rotate(glm::vec3(-offset.y / 30,0,0))
                                                                .Rotate(glm::vec3(0,-offset.x / 30,0));
                }
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

                for (int i = 0; i < m_pLight->GetLightNum(); i++) {
                    glm::vec3 dir = m_pLight->GetLightTransformation(i).GetFrontDir();
                    glm::vec3 move = dir * -offset.y / 100.f;
                    m_pLight->GetLightTransformation(i).Translate(move);
                }

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
