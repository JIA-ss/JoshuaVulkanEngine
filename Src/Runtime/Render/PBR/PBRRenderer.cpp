#include "PBRRenderer.h"
#include "Runtime/Platform/PlatformInputMonitor.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Prefilter/Ibl.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDepthStencilState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <Util/Modelutil.h>
#include <Util/Fileutil.h>
#include <Util/Mathutil.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <stdint.h>
using namespace Render;

PBRRenderer::PBRRenderer(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, physicalConfig)
{

}

PBRRenderer::~PBRRenderer()
{
    m_pDevice->GetVkDevice().waitIdle();
    m_pRenderPass = nullptr;
    m_pModel.reset();
}

void PBRRenderer::prepare()
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

    // prepareIbl
    prepareIbl();


}

void PBRRenderer::render()
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

    m_pCamera->UpdateUniformBuffer(m_imageIdx);
    m_pLight->UpdateLightUBO(m_imageIdx);

    // reset fence after acquiring the image
    m_pDevice->GetVkDevice().resetFences(m_vkFences[m_frameIdxInFlight]);

    // record command buffer
    m_vkCmds[m_frameIdxInFlight].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_vkCmds[m_frameIdxInFlight].begin(beginInfo);
    {
        m_pPipelineLayout->PushConstantT(m_vkCmds[m_frameIdxInFlight], 0, m_pushConstant, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

        std::vector<vk::DescriptorSet> tobinding;
        m_pIbl->FillToBindingDescriptorSets(tobinding);

        std::vector<vk::ClearValue> clears(2);
        clears[0] = vk::ClearValue{vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,0.0f,1.0f}}};
        clears[1] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
        if (m_pPhysicalDevice->IsUsingMSAA())
        {
            clears.push_back(clears[0]);
        }
        m_pRenderPass->Begin(m_vkCmds[m_frameIdxInFlight], clears, vk::Rect2D{vk::Offset2D{0,0}, m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent}, m_pDevice->GetVulkanPresentFramebuffer(m_imageIdx)->GetVkFramebuffer());
        {
            auto& extent = m_pDevice->GetPVulkanSwapchain()->GetSwapchainInfo().imageExtent;
            vk::Rect2D rect{{0,0},extent};
            m_vkCmds[m_frameIdxInFlight].setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
            m_vkCmds[m_frameIdxInFlight].setScissor(0,rect);

            m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "skybox");
            m_pSkyboxModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);


            m_pRenderPass->BindGraphicPipeline(m_vkCmds[m_frameIdxInFlight], "default");
            m_pModel->Draw(m_vkCmds[m_frameIdxInFlight], m_pPipelineLayout.get(), tobinding, m_frameIdxInFlight);

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

void PBRRenderer::prepareModel()
{
    m_pModel = RHI::ModelPresets::CreateCerberusPBRModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get());
    auto& transformation = m_pModel->GetTransformation();
    transformation.SetRotation(glm::vec3(0,90,90));
    m_pSkyboxModel = RHI::ModelPresets::CreateSkyboxModel(m_pDevice.get(), m_pSet1SamplerSetLayout.lock().get());

    std::array<std::vector<RHI::Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT> uboInfos;
    auto camUbo = m_pCamera->GetUboInfo();
    auto lightUbo = m_pLight->GetUboInfo();
    for (int frameId = 0; frameId < uboInfos.size(); frameId++)
    {
        uboInfos[frameId].push_back(camUbo[frameId]);
        uboInfos[frameId].push_back(lightUbo[frameId]);
    }
    m_pModel->InitUniformDescriptorSets(uboInfos);
    m_pSkyboxModel->InitUniformDescriptorSets(uboInfos);
}

void PBRRenderer::prepareCamera()
{
    auto extent = m_pDevice->GetSwapchainExtent();
    float aspect = extent.width / (float) extent.height;
    m_pCamera.reset(new Camera(45.f, aspect, 0.1f, 500.f));
    m_pCamera->GetVPMatrix().SetPosition(glm::vec3(0,0,2));

    m_pCamera->InitUniformBuffer(m_pDevice.get());
}

void PBRRenderer::prepareLight()
{
    m_pLight.reset(new Lights(m_pDevice.get(), 5));
    m_pLight->GetLightTransformation(0).SetPosition(glm::vec3(0,0, 2));
    m_pLight->GetLightTransformation(1).SetPosition(glm::vec3(50,0, 2));
    m_pLight->GetLightTransformation(2).SetPosition(glm::vec3(100,0, 2));
    m_pLight->GetLightTransformation(3).SetPosition(glm::vec3(0,-40, 2));
    m_pLight->GetLightTransformation(4).SetPosition(glm::vec3(50,-40, 2));

    for (int i = 0; i < 5; i++)
    {
        m_pLight->SetLightColor(glm::vec4(1.0f), i);
    }
}

void PBRRenderer::prepareInputCallback()
{
    static bool BtnLeftPressing = false;
    static bool BtnRightPressing = false;
    static bool LeftShiftPressing = false;
    static bool LeftControlPressing = false;

    auto inputMonitor = m_pPhysicalDevice->GetPWindow()->GetInputMonitor();

    inputMonitor->AddScrollCallback([&](double xoffset, double yoffset){
        if (LeftShiftPressing)
        {
            glm::vec3 dir = m_pLight->GetLightTransformation().GetFrontDir();
            glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));
            glm::vec3 move = dir * (float)yoffset;
            xoffset = -xoffset;
            move += right * (float)xoffset;
            for (int i = 0; i < 5; i++)
            {
                m_pLight->GetLightTransformation(i).Translate(move);
            }
        }
        else if (LeftControlPressing)
        {
            glm::vec3 dir = m_pCamera->GetDirection();
            glm::vec3 right = m_pCamera->GetRight();
            glm::vec3 move = dir * (float)yoffset;
            xoffset = -xoffset;
            move += right * (float)xoffset;
            m_pModel->GetTransformation().Translate(move);
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

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::W, [&](){
        for (int i = 0; i < 5; i++)
        {
            auto position = m_pLight->GetLightTransformation(i).GetPosition();
            m_pLight->GetLightTransformation(i).SetPosition(position + glm::vec3(0,-1,0));
        }
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::A, [&](){
        for (int i = 0; i < 5; i++) {
            auto position = m_pLight->GetLightTransformation(i).GetPosition();
            m_pLight->GetLightTransformation(i).SetPosition(position + glm::vec3(-1,0,0));
        }
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::S, [&](){
        for (int i = 0; i < 5; i++) {
            auto position = m_pLight->GetLightTransformation(i).GetPosition();
            m_pLight->GetLightTransformation(i).SetPosition(position + glm::vec3(0,1,0));
        }
    });
    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::D, [&](){
        for (int i = 0; i < 5; i++) {
            auto position = m_pLight->GetLightTransformation(i).GetPosition();
            m_pLight->GetLightTransformation(i).SetPosition(position + glm::vec3(1,0,0));
        }
    });


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::SPACE, [&](){
        std::cout << "Cam Pos: " << glm::to_string(m_pCamera->GetPosition()) << std::endl;
        std::cout << "Cam Rot: " << glm::to_string(m_pCamera->GetVPMatrix().GetRotation()) << std::endl;

        std::cout << "Light Pos: " << glm::to_string(m_pLight->GetLightTransformation(0).GetPosition()) << std::endl;
        std::cout << "Light Rot: " << glm::to_string(m_pLight->GetLightTransformation(0).GetRotation()) << std::endl;
    });


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::TAB, [&](){
        m_pushConstant.vec4.x = 1 - m_pushConstant.vec4.x;
    });


    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::LEFT_SHIFT, [&](){
        LeftShiftPressing = true;
    });
    inputMonitor->AddKeyboardUpCallback(platform::Keyboard::Key::LEFT_SHIFT, [&](){
        LeftShiftPressing = false;
    });

    inputMonitor->AddKeyboardPressedCallback(platform::Keyboard::Key::LEFT_CONTROL, [&](){
        LeftControlPressing = true;
    });
    inputMonitor->AddKeyboardUpCallback(platform::Keyboard::Key::LEFT_CONTROL, [&](){
        LeftControlPressing = false;
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
                for (int i = 0; i < 5; i++) {
                    m_pLight->GetLightTransformation(i).Rotate(glm::vec3(-offset.y / 30,0,0))
                                                                .Rotate(glm::vec3(0,-offset.x / 30,0));
                }
            }
            else if (LeftControlPressing)
            {
                m_pModel->GetTransformation().Rotate(glm::vec3(-offset.y / 30,0,0))
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
                for (int i = 0; i < 5; i++) {
                    glm::vec3 dir = m_pLight->GetLightTransformation(i).GetFrontDir();
                    glm::vec3 move = dir * -offset.y / 100.f;
                    m_pLight->GetLightTransformation(i).Translate(move);
                }
            }
            else if (LeftControlPressing)
            {
                glm::vec3 dir = m_pCamera->GetDirection();
                glm::vec3 move = dir * -offset.y / 100.f;
                m_pModel->GetTransformation().Translate(move);
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
void PBRRenderer::prepareIbl()
{
    m_pIbl.reset(new Prefilter::Ibl(m_pDevice.get()));
    m_pIbl->Prepare();
}


void PBRRenderer::prepareLayout()
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


    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice.get(),
            {m_pSet0UniformSetLayout.lock(), m_pSet1SamplerSetLayout.lock(), m_pSet2ShadowmapSamplerLayout.lock()}
            ,pushconstants
            )
        );
}

void PBRRenderer::preparePipeline()
{
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/pbr.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/pbr.frag.spv", vk::ShaderStageFlagBits::eFragment);
    auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                            .SetshaderSet(shaderSet)
                            .SetVulkanPipelineLayout(m_pPipelineLayout)
                            .buildUnique();
    m_pRenderPass->AddGraphicRenderPipeline("default", std::move(pipeline));


    std::shared_ptr<RHI::VulkanShaderSet> skyboxShaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice.get());
    skyboxShaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/skybox.vert.spv", vk::ShaderStageFlagBits::eVertex);
    skyboxShaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/skybox.frag.spv", vk::ShaderStageFlagBits::eFragment);

    {
        RHI::VulkanDepthStencilState::Config depthConfig;
        depthConfig.DepthWriteEnable = VK_FALSE;
        depthConfig.DepthTestEnable = VK_FALSE;
        depthConfig.DepthCompareOp = vk::CompareOp::eLessOrEqual;
        auto depthStencilState = std::make_shared<RHI::VulkanDepthStencilState>(depthConfig);

        auto raster = RHI::VulkanRasterizationStateBuilder()
                    .SetCullMode(vk::CullModeFlagBits::eNone)
                    .build();

        pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice.get(), m_pRenderPass.get())
                    .SetVulkanDepthStencilState(depthStencilState)
                    .SetVulkanRasterizationState(raster)
                    .SetshaderSet(skyboxShaderSet)
                    .SetVulkanPipelineLayout(m_pPipelineLayout)
                    .buildUnique();
    }
    m_pRenderPass->AddGraphicRenderPipeline("skybox", std::move(pipeline));
}
