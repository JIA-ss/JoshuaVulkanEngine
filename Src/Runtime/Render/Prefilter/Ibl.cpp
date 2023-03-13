#include "Ibl.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Graphic/ModelPresets.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Util/Fileutil.h"
#include "Util/Modelutil.h"
#include "Util/Textureutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <assimp/material.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <utility>
#include <vector>

namespace Render { namespace Prefilter {

void Ibl::Prepare()
{
    prepareLayout();
    prepareCamera();
    prepareSamplerCubeModel();
    prepareCmd();
    generateIrradianceCubeMap();
    generatePrefilterEnvCubeMap();
    generateBrdfLUT();


    generateOutputDiscriptorSet();
}


void Ibl::FillToBindingDescriptorSets(std::vector<vk::DescriptorSet>& tobinding)
{
    if (tobinding.size() <= 2)
    {
        tobinding.resize(3);
    }
    tobinding[2] = m_pDescriptor->GetVkDescriptorSet(0);
}


void Ibl::prepareLayout()
{
    auto SET0 = m_pDevice->GetDescLayoutPresets().UBO;
        // BINDING0 CameraUniformBufferObject
        // BINDING1 LightInforUniformBufferObject
        // BINDING2 ModelUniformBufferObject
    auto SET1 = m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER;
        // BINDING1~5 Sampler

    std::map<int, vk::PushConstantRange> Constant
    {
        {
            0, //offset
            vk::PushConstantRange
            {
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(PushConstant)
            }
        }
    };
    m_pPipelineLayout.reset(new RHI::VulkanPipelineLayout(m_pDevice,{SET0, SET1},Constant));
}

void Ibl::prepareCamera()
{
    auto window_setting = m_pDevice->GetVulkanPhysicalDevice()->GetPWindow()->GetWindowSetting();
    m_pCamera.reset(new Camera(90, 1.0f, 0.1f, 512.f));
    m_pCamera->InitUniformBuffer(m_pDevice);
}

void Ibl::prepareSamplerCubeModel()
{
    m_pLight.reset(new Lights(m_pDevice, 1));
    m_pSamplerCubeModel = RHI::ModelPresets::CreateSkyboxModel(m_pDevice, m_pPipelineLayout->GetPVulkanDescriptorSet(1));

    std::array<std::vector<RHI::Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT> uboInfos;
    auto camUbo = m_pCamera->GetUboInfo();
    auto lightUbo = m_pLight->GetUboInfo();
    for (int frameId = 0; frameId < uboInfos.size(); frameId++)
    {
        uboInfos[frameId].push_back(camUbo[frameId]);
        uboInfos[frameId].push_back(lightUbo[frameId]);
    }
    m_pSamplerCubeModel->InitUniformDescriptorSets(uboInfos);
}

void Ibl::prepareCmd()
{
    m_pCmdPool.reset(new RHI::VulkanCommandPool(m_pDevice, m_pDevice->GetQueueFamilyIndices().graphic.value()));
}

void Ibl::generateIrradianceCubeMap()
{
    auto tStart = std::chrono::high_resolution_clock::now();

    const vk::Format format = vk::Format::eR32G32B32A32Sfloat;
    const int32_t dim = 64;
    const uint32_t numMips = floor(log2(dim)) + 1;
    float deltaPhi = (2.0f * 3.14159265358979323846f) / 180.0f;
    float deltaTheta = (0.5f * 3.14159265358979323846f) / 64.0f;

    RHI::VulkanImageResource::Config cubemapResourceConfig = RHI::VulkanImageResource::Config::CubeMap(dim, dim, numMips);
    RHI::VulkanImageSampler::Config cubemapSamplerConfig = RHI::VulkanImageSampler::Config::CubeMap(numMips);
    cubemapResourceConfig.format = format;

    if (!m_pIrradianceCubeMapSampler)
    {
        m_pIrradianceCubeMapSampler.reset(new RHI::VulkanImageSampler(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, cubemapSamplerConfig, cubemapResourceConfig));
    }

    // irrandiance sampler render pass
    if (!m_pIrradianceRenderPass)
    {
        vk::AttachmentDescription attDesc;
        attDesc.setFormat(format)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference attRef;
        attRef.setAttachment(0)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        vk::SubpassDescription subpassDesc;
        subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setColorAttachments(attRef);
        std::array<vk::SubpassDependency, 2> dependencies;
        dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
                        .setDstSubpass(0)
                        .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
                        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
        dependencies[1].setSrcSubpass(0)
                        .setDstSubpass(VK_SUBPASS_EXTERNAL)
                        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
                        .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
        vk::RenderPassCreateInfo rpcreateInfo;
        rpcreateInfo.setAttachments(attDesc)
                    .setSubpasses(subpassDesc)
                    .setDependencies(dependencies);
        vk::RenderPass rp = m_pDevice->GetVkDevice().createRenderPass(rpcreateInfo);
        m_pIrradianceRenderPass.reset(new RHI::VulkanRenderPass(m_pDevice, rp));
    }

    // frame buffer
    if (!m_pIrradianceFramebuffer)
    {
        m_pIrradianceFramebuffer.reset(new Framebuffer{});

        RHI::VulkanImageSampler::Config samplerConfig;
        RHI::VulkanImageResource::Config imageResourceConfig;

        imageResourceConfig.extent = vk::Extent3D{ dim, dim, 1 };
        imageResourceConfig.miplevel = 1;
        imageResourceConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
        imageResourceConfig.format = format;

        m_pIrradianceFramebuffer->attachment.reset(new RHI::VulkanImageResource(m_pDevice, vk::MemoryPropertyFlagBits::eDeviceLocal,imageResourceConfig));

        vk::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.setRenderPass(m_pIrradianceRenderPass->GetVkRenderPass())
                    .setAttachments(m_pIrradianceFramebuffer->attachment->GetVkImageView())
                    .setWidth(dim)
                    .setHeight(dim)
                    .setLayers(1)
                    ;
        m_pIrradianceFramebuffer->native = m_pDevice->GetVkDevice().createFramebuffer(fbCreateInfo);

        // transfer imageresource layout
        m_pIrradianceFramebuffer->attachment->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    }

    // pipeline
    if (!m_pIrradianceRenderPass->GetGraphicRenderPipeline("irrandiance"))
    {
        std::shared_ptr<RHI::VulkanShaderSet> shader = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
        shader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/ibl.filtercube.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/ibl.irradiance.frag.spv", vk::ShaderStageFlagBits::eFragment);
        auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice, m_pIrradianceRenderPass.get())
                    .SetshaderSet(shader)
                    .SetVulkanPipelineLayout(m_pPipelineLayout)
                    .SetVulkanMultisampleState(std::make_shared<RHI::VulkanMultisampleState>(vk::SampleCountFlagBits::e1))
                    .buildUnique();
        m_pIrradianceRenderPass->AddGraphicRenderPipeline("irrandiance", std::move(pipeline));
    }

    // render
    {
        vk::CommandBuffer cmd = m_pCmdPool->BeginSingleTimeCommand();
        vk::ClearValue clear { vk::ClearColorValue{std::array<float, 4>{{0.0f, 0.0f, 0.2f, 0.0f}}} };
        vk::Rect2D renderArea {0, vk::Extent2D{dim, dim}};

        // cannot use uniformbuffer, because it will be covered
        // std::array<glm::vec3, 6> rotations
        // {
        //     glm::vec3(180, 90, 0),
        //     glm::vec3(180, -90, 0),
        //     glm::vec3(-90, 0, 0),
        //     glm::vec3(90, 0, 0),
        //     glm::vec3(180, 0, 0),
        //     glm::vec3(0, 0, 180),
        // };

		std::vector<glm::mat4> matrices = {
			// POSITIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

        vk::Viewport viewport {0,0,dim,dim,0,1};
        vk::Rect2D scissor {{0,0}, vk::Extent2D{dim, dim}};
        cmd.setViewport(0,1,&viewport);
        cmd.setScissor(0,scissor);

        m_pCamera->UpdateUniformBuffer(0);

        // Change image layout for all cubemap faces to transfer destination
        m_pIrradianceCubeMapSampler->GetPImageResource()->TransitionImageLayout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        // render 6 faces for cubemap
        for (uint32_t m = 0; m < numMips; m++)
        {
            for (uint32_t f = 0; f < 6; f++)
            {
                viewport.width = (float)dim * std::pow(0.5f, m);
                viewport.height = (float)dim * std::pow(0.5f, m);
                cmd.setViewport(0,1,&viewport);

                m_pIrradianceRenderPass->Begin(cmd, {clear}, renderArea, m_pIrradianceFramebuffer->native);
                {
                    // m_pSamplerCubeModel->GetTransformation().SetRotation(rotations[f]);

                    PushConstant consts;
                    consts.mvp = matrices[f];
                    consts.deltaPhi = deltaPhi;
                    consts.deltaTheta = deltaTheta;
                    m_pPipelineLayout->PushConstantT<PushConstant>(cmd, 0, consts, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
                    m_pIrradianceRenderPass->BindGraphicPipeline(cmd, "irrandiance");
                    std::vector<vk::DescriptorSet> tobinding;
                    m_pSamplerCubeModel->Draw(cmd, m_pPipelineLayout.get(), tobinding, 0);
                }
                m_pIrradianceRenderPass->End(cmd);


                // change attachment layout to transferSrc
                m_pIrradianceFramebuffer->attachment->TransitionImageLayout(cmd, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);
                // copy
                vk::ImageCopy copyRegion;
                copyRegion.setSrcSubresource(
                    vk::ImageSubresourceLayers()
                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                        .setBaseArrayLayer(0)
                                        .setMipLevel(0)
                                        .setLayerCount(1))
                            .setSrcOffset({0,0,0})
                            .setDstSubresource(
                                    vk::ImageSubresourceLayers()
                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                        .setBaseArrayLayer(f)
                                        .setMipLevel(m)
                                        .setLayerCount(1))
                            .setDstOffset({0,0,0})
                            .setExtent(vk::Extent3D{(uint32_t)viewport.width, (uint32_t)viewport.height, 1});
                m_pIrradianceFramebuffer->attachment->CopyTo(cmd, m_pIrradianceCubeMapSampler->GetPImageResource(), copyRegion);

                // change attachment layout to colorAttachment
                m_pIrradianceFramebuffer->attachment->TransitionImageLayout(cmd, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal);
            }
        }

        // change sampler layout to shaderReadOnly
        m_pIrradianceCubeMapSampler->GetPImageResource()->TransitionImageLayout(cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        m_pCmdPool->EndSingleTimeCommand(cmd, m_pDevice->GetVkGraphicQueue());
    }


    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating irradiance cube with " << numMips << " mip levels took " << tDiff << " ms" << std::endl;
}


void Ibl::generatePrefilterEnvCubeMap()
{
    auto tStart = std::chrono::high_resolution_clock::now();

    const vk::Format format = vk::Format::eR16G16B16A16Sfloat;
    const int32_t dim = 512;
    const uint32_t numMips = floor(log2(dim)) + 1;

    RHI::VulkanImageResource::Config prefilterCubemapResourceConfig = RHI::VulkanImageResource::Config::CubeMap(dim, dim, numMips);
    RHI::VulkanImageSampler::Config prefilterCubemapSamplerConfig = RHI::VulkanImageSampler::Config::CubeMap(numMips);
    prefilterCubemapResourceConfig.format = format;

    if (!m_pPrefilterEnvCubeMapSampler)
    {
        m_pPrefilterEnvCubeMapSampler.reset(new RHI::VulkanImageSampler(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, prefilterCubemapSamplerConfig, prefilterCubemapResourceConfig));
    }

    // irrandiance sampler render pass
    if (!m_pPrefilterEnvRenderPass)
    {
        vk::AttachmentDescription attDesc;
        attDesc.setFormat(format)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference attRef;
        attRef.setAttachment(0)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        vk::SubpassDescription subpassDesc;
        subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setColorAttachments(attRef);
        std::array<vk::SubpassDependency, 2> dependencies;
        dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
                        .setDstSubpass(0)
                        .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
                        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
        dependencies[1].setSrcSubpass(0)
                        .setDstSubpass(VK_SUBPASS_EXTERNAL)
                        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
                        .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
        vk::RenderPassCreateInfo rpcreateInfo;
        rpcreateInfo.setAttachments(attDesc)
                    .setSubpasses(subpassDesc)
                    .setDependencies(dependencies);
        vk::RenderPass rp = m_pDevice->GetVkDevice().createRenderPass(rpcreateInfo);
        m_pPrefilterEnvRenderPass.reset(new RHI::VulkanRenderPass(m_pDevice, rp));
    }

    // frame buffer
    if (!m_pPrefilterEnvFramebuffer)
    {
        m_pPrefilterEnvFramebuffer.reset(new Framebuffer{});

        RHI::VulkanImageSampler::Config samplerConfig;
        RHI::VulkanImageResource::Config imageResourceConfig;

        imageResourceConfig.extent = vk::Extent3D{ dim, dim, 1 };
        imageResourceConfig.miplevel = 1;
        imageResourceConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
        imageResourceConfig.format = format;

        m_pPrefilterEnvFramebuffer->attachment.reset(new RHI::VulkanImageResource(m_pDevice, vk::MemoryPropertyFlagBits::eDeviceLocal,imageResourceConfig));

        vk::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.setRenderPass(m_pPrefilterEnvRenderPass->GetVkRenderPass())
                    .setAttachments(m_pPrefilterEnvFramebuffer->attachment->GetVkImageView())
                    .setWidth(dim)
                    .setHeight(dim)
                    .setLayers(1)
                    ;
        m_pPrefilterEnvFramebuffer->native = m_pDevice->GetVkDevice().createFramebuffer(fbCreateInfo);

        // transfer imageresource layout
        m_pPrefilterEnvFramebuffer->attachment->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
    }

    // pipeline
    if (!m_pPrefilterEnvRenderPass->GetGraphicRenderPipeline("prefilterEnvironment"))
    {
        std::shared_ptr<RHI::VulkanShaderSet> shader = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
        shader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/ibl.filtercube.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shader->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/ibl.prefilterEnvMap.frag.spv", vk::ShaderStageFlagBits::eFragment);
        auto pipeline = RHI::VulkanRenderPipelineBuilder(m_pDevice, m_pPrefilterEnvRenderPass.get())
                    .SetshaderSet(shader)
                    .SetVulkanPipelineLayout(m_pPipelineLayout)
                    .SetVulkanMultisampleState(std::make_shared<RHI::VulkanMultisampleState>(vk::SampleCountFlagBits::e1))
                    .buildUnique();
        m_pPrefilterEnvRenderPass->AddGraphicRenderPipeline("prefilterEnvironment", std::move(pipeline));
    }

    // render
    {
        vk::CommandBuffer cmd = m_pCmdPool->BeginSingleTimeCommand();
        vk::ClearValue clear { vk::ClearColorValue{std::array<float, 4>{{0.0f, 0.0f, 0.2f, 0.0f}}} };
        vk::Rect2D renderArea {0, vk::Extent2D{dim, dim}};

        // cannot use uniformbuffer, because it will be covered
        // std::array<glm::vec3, 6> rotations
        // {
        //     glm::vec3(180, 90, 0),
        //     glm::vec3(180, -90, 0),
        //     glm::vec3(-90, 0, 0),
        //     glm::vec3(90, 0, 0),
        //     glm::vec3(180, 0, 0),
        //     glm::vec3(0, 0, 180),
        // };

		std::vector<glm::mat4> matrices = {
			// POSITIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

        vk::Viewport viewport {0,0,dim,dim,0,1};
        vk::Rect2D scissor {{0,0}, vk::Extent2D{dim, dim}};
        cmd.setViewport(0,1,&viewport);
        cmd.setScissor(0,scissor);

        m_pCamera->UpdateUniformBuffer(0);

        // Change image layout for all cubemap faces to transfer destination
        m_pPrefilterEnvCubeMapSampler->GetPImageResource()->TransitionImageLayout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);


        PushConstant consts;
        consts.numSamples = 32.0f;
        // render 6 faces for cubemap
        for (uint32_t m = 0; m < numMips; m++)
        {
            consts.roughness = (float)m / (float)(numMips - 1);
            for (uint32_t f = 0; f < 6; f++)
            {
                viewport.width = (float)dim * std::pow(0.5f, m);
                viewport.height = (float)dim * std::pow(0.5f, m);
                cmd.setViewport(0,1,&viewport);

                m_pPrefilterEnvRenderPass->Begin(cmd, {clear}, renderArea, m_pPrefilterEnvFramebuffer->native);
                {
                    // m_pSamplerCubeModel->GetTransformation().SetRotation(rotations[f]);


                    consts.mvp = matrices[f];
                    m_pPipelineLayout->PushConstantT<PushConstant>(cmd, 0, consts, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
                    m_pPrefilterEnvRenderPass->BindGraphicPipeline(cmd, "prefilterEnvironment");
                    std::vector<vk::DescriptorSet> tobinding;
                    m_pSamplerCubeModel->Draw(cmd, m_pPipelineLayout.get(), tobinding, 0);
                }
                m_pPrefilterEnvRenderPass->End(cmd);


                // change attachment layout to transferSrc
                m_pPrefilterEnvFramebuffer->attachment->TransitionImageLayout(cmd, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);
                // copy
                vk::ImageCopy copyRegion;
                copyRegion.setSrcSubresource(
                    vk::ImageSubresourceLayers()
                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                        .setBaseArrayLayer(0)
                                        .setMipLevel(0)
                                        .setLayerCount(1))
                            .setSrcOffset({0,0,0})
                            .setDstSubresource(
                                    vk::ImageSubresourceLayers()
                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                        .setBaseArrayLayer(f)
                                        .setMipLevel(m)
                                        .setLayerCount(1))
                            .setDstOffset({0,0,0})
                            .setExtent(vk::Extent3D{(uint32_t)viewport.width, (uint32_t)viewport.height, 1});
                m_pPrefilterEnvFramebuffer->attachment->CopyTo(cmd, m_pPrefilterEnvCubeMapSampler->GetPImageResource(), copyRegion);

                // change attachment layout to colorAttachment
                m_pPrefilterEnvFramebuffer->attachment->TransitionImageLayout(cmd, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal);
            }
        }

        // change sampler layout to shaderReadOnly
        m_pPrefilterEnvCubeMapSampler->GetPImageResource()->TransitionImageLayout(cmd, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        m_pCmdPool->EndSingleTimeCommand(cmd, m_pDevice->GetVkGraphicQueue());
    }


    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "Generating prefilter environment cubemap with " << numMips << " mip levels took " << tDiff << " ms" << std::endl;
}

void Ibl::generateBrdfLUT()
{
    auto rawdata = Util::Texture::RawData::Load(Util::File::getResourcePath() / "Texture/ibl_brdf_lut.png", Util::Texture::RawData::Format::eRgbAlpha);
    RHI::VulkanImageSampler::Config samplerConfig;
    RHI::VulkanImageResource::Config imageConfig;

    imageConfig.extent = vk::Extent3D{(uint32_t)rawdata->GetWidth(), (uint32_t)rawdata->GetHeight(), 1};
    m_pBrdfLUTSampler.reset(new RHI::VulkanImageSampler(m_pDevice, rawdata, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig, imageConfig));
}

void Ibl::generateOutputDiscriptorSet()
{
    std::vector<vk::DescriptorPoolSize> sizes
    {
        vk::DescriptorPoolSize {vk::DescriptorType::eCombinedImageSampler, 3}
    };
    m_pDescPool.reset(new RHI::VulkanDescriptorPool(m_pDevice, sizes, 3));

    m_pDescriptor = m_pDescPool->AllocSamplerDescriptorSet(
        m_pPipelineLayout->GetPVulkanDescriptorSet(1),
        {
            m_pIrradianceCubeMapSampler.get(),
            m_pPrefilterEnvCubeMapSampler.get(),
            m_pBrdfLUTSampler.get()
            },
            {
                1,2,3
            }
    );
}

}}