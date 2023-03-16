#include "ShadowMapRenderPass.h"
#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDepthStencilState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanDynamicState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_USING

ShadowMapRenderPass::ShadowMapRenderPass(VulkanDevice* device, int num, uint32_t width, uint32_t height)
    : m_num(num)
    , m_width(width)
    , m_height(height)
    , m_pDevice(device)
{
    ZoneScopedN("ShadowMapRenderPass::ShadowMapRenderPass");
    m_vkDepthFormat = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    // No.Light <==> (FRAMES) * No.Framebuffer <==> No.DepthSampler
    initDepthSampler();

    // No.Light <==> No.RenderPass
    initRenderPass();

    initFramebuffer();

    // ALL.DepthSampler <==> (1)DepthSamplerDescriptorSets
    initDepthSamplerDescriptor();
    // No.Light <==> (FRAMES) * No.UniformBuffer <==> (1)UniformBuffer
    initUniformBuffer();
    initPipelines();
}


ShadowMapRenderPass::~ShadowMapRenderPass()
{
    m_pDepthSamplerDescriptorSets = nullptr;

    m_pDepthSamplers.clear();
    m_pVulkanFramebuffers.clear();
    m_uniformBuffers.clear();

    m_pDescriptorPool.reset();
    m_pRenderPasses.clear();
}

void ShadowMapRenderPass::InitModelShadowDescriptor(Model* model)
{
    ZoneScopedN("ShadowMapRenderPass::InitModelShadowDescriptor");
    for (int lightId = 0; lightId < m_num; lightId++)
    {
        std::vector<RHI::Model::UBOLayoutInfo> uboInfos;
        int bindingId = VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID;
        uboInfos.emplace_back(RHI::Model::UBOLayoutInfo
        {
            m_uniformBuffers[lightId].get(), VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID, sizeof(CameraUniformBufferObject)
        });
        model->InitShadowPassUniforDescriptorSets(uboInfos, lightId);
    }
}

void ShadowMapRenderPass::SetShadowPassLightVPUBO(CameraUniformBufferObject& ubo, int lightIdx)
{
    m_uniformBuffers[lightIdx]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
}

void ShadowMapRenderPass::FillDepthSamplerToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout)
{
    m_pDepthSamplerDescriptorSets->FillToBindedDescriptorSetsVector(descList, pipelineLayout);
}

void ShadowMapRenderPass::Render(vk::CommandBuffer cmd, std::vector<Model*> models)
{
    ZoneScopedN("ShadowMapRenderPass::Render");
    std::vector<vk::ClearValue> clears(1);
    clears[0] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
    vk::Extent2D extent = vk::Extent2D{m_width, m_height};
    for (int lightIdx = 0; lightIdx < m_num; lightIdx++)
    {
        m_pRenderPasses[lightIdx]->Begin(cmd, clears, vk::Rect2D{vk::Offset2D{0,0}, extent}, m_pVulkanFramebuffers[lightIdx]->GetVkFramebuffer());
        {
            ZoneScopedN("ShadowMapRenderPass::Render::renderpass recording");
            m_pRenderPasses[lightIdx]->BindGraphicPipeline(cmd, "shadowmap");
            // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pPipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});

            vk::Rect2D rect{{0,0},extent};
            cmd.setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
            cmd.setScissor(0,rect);
            cmd.setDepthBias(DEPTH_BIAS_CONSTANT, 0.0f, DEPTH_BIAS_SLOP);
            for (auto& model : models)
            {
                model->DrawShadowPass(cmd, m_pPipelineLayout.get(), lightIdx);
            }
        }
        //cmd.setEvent(m_vkEvents[frameId][lightIdx], vk::PipelineStageFlagBits::eEarlyFragmentTests);
        m_pRenderPasses[lightIdx]->End(cmd);
    }


    //cmd.waitEvents(m_vkEvents[frameId], vk::PipelineStageFlagBits::eVertexShader, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {});

}


// No.Framebuffer <==> No.DepthSampler
void ShadowMapRenderPass::initDepthSampler()
{
    VulkanImageSampler::Config samplerConfig;
    VulkanImageResource::Config imageConfig;

    imageConfig.imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageConfig.format = m_vkDepthFormat;
    imageConfig.extent = vk::Extent3D{(uint32_t)m_width, (uint32_t)m_height, 1};
    imageConfig.initialLayout = vk::ImageLayout::eUndefined;
    imageConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                .setBaseMipLevel(0)
                                .setLevelCount(1)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1);

    samplerConfig.uAddressMode = vk::SamplerAddressMode::eClampToBorder;
    samplerConfig.vAddressMode = vk::SamplerAddressMode::eClampToBorder;
    samplerConfig.wAddressMode = vk::SamplerAddressMode::eClampToBorder;
    samplerConfig.maxLod = 1;
    samplerConfig.anisotropyEnable = VK_TRUE;
    samplerConfig.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    samplerConfig.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;

    m_pDepthSamplers.resize(m_num);
    for (int lightIdx = 0; lightIdx < m_num; lightIdx++)
    {
        m_pDepthSamplers[lightIdx].reset(new VulkanImageSampler(
            m_pDevice, nullptr,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            samplerConfig,
            imageConfig)
        );
        // m_pDepthSamplers[frameId]->GetPImageResource()->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
    }

}

// No.Light <==> No.RenderPass
void ShadowMapRenderPass::initRenderPass()
{
    m_pRenderPasses.resize(m_num);
    for (int i = 0; i < m_num; i++)
    {
        std::vector<VulkanFramebuffer::Attachment> fbAttachments(1);
        fbAttachments[0].resourceFormat = m_vkDepthFormat;
        fbAttachments[0].samples = vk::SampleCountFlagBits::e1;
        fbAttachments[0].type = VulkanFramebuffer::kDepthStencil;
        fbAttachments[0].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        fbAttachments[0].resourceFinalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        fbAttachments[0].resource = m_pDepthSamplers[i]->GetPImageResource()->GetNative();

        m_pRenderPasses[i] = VulkanRenderPassBuilder(m_pDevice)
                                .SetAttachments(fbAttachments)
                                .SetDefaultSubpass()
                                .AddSubpassDependency(
                                    vk::SubpassDependency()
                                                .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                                                .setDstSubpass(0)
                                                .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                                .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
                                                .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                                                .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                                                .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                                .AddSubpassDependency(
                                    vk::SubpassDependency()
                                                .setSrcSubpass(0)
                                                .setDstSubpass(VK_SUBPASS_EXTERNAL)
                                                .setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
                                                .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                                .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                                                .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                                                .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                                .buildUnique();
    }
}

// No.Light <==> (FRAMES) * No.Framebuffer
void ShadowMapRenderPass::initFramebuffer()
{
    m_pVulkanFramebuffers.resize(m_num);
    for (int lightId = 0; lightId < m_num; lightId++)
    {
        std::vector<VulkanFramebuffer::Attachment> fbAttachments(1);
        fbAttachments[0].resourceFormat = m_vkDepthFormat;
        fbAttachments[0].samples = vk::SampleCountFlagBits::e1;
        fbAttachments[0].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        fbAttachments[0].resourceFinalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        fbAttachments[0].resource = m_pDepthSamplers[lightId]->GetPImageResource()->GetNative();

        m_pVulkanFramebuffers[lightId].reset(new VulkanFramebuffer(m_pDevice, m_pRenderPasses[lightId].get(), m_width, m_height, 1, fbAttachments));
    }
}

// ALL.DepthSampler <==> (1)DepthSamplerDescriptorSets
void ShadowMapRenderPass::initDepthSamplerDescriptor()
{
    std::vector<vk::DescriptorPoolSize> poolSizes =
    {
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, VulkanDescriptorSetLayout::DESCRIPTOR_SHADOWMAP5_BINDING_ID * MAX_FRAMES_IN_FLIGHT },
        vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, (uint32_t)(m_num + 1) * MAX_FRAMES_IN_FLIGHT }
    };
    uint32_t max = std::max(poolSizes[0].descriptorCount, poolSizes[1].descriptorCount);
    m_pDescriptorPool.reset(new VulkanDescriptorPool(m_pDevice, poolSizes, max));


    std::vector<VulkanImageSampler*> samplers;
    std::vector<uint32_t> binding;
    for (uint32_t bindingId = VulkanDescriptorSetLayout::DESCRIPTOR_SHADOWMAP1_BINDING_ID; bindingId <= VulkanDescriptorSetLayout::DESCRIPTOR_SHADOWMAP5_BINDING_ID; bindingId++)
    {
        samplers.push_back(m_pDepthSamplers[bindingId < m_num ? bindingId : m_num - 1].get());
        binding.push_back(bindingId);
    }
    m_pDepthSamplerDescriptorSets = m_pDescriptorPool->AllocSamplerDescriptorSet(m_pDevice->GetDescLayoutPresets().SHADOWMAP.get(), samplers, binding, vk::ImageLayout::eDepthStencilReadOnlyOptimal, 1);
}

void ShadowMapRenderPass::initPipelines()
{
    // only vertex
    std::shared_ptr<RHI::VulkanShaderSet> shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
    shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/shadowmap.vert.spv", vk::ShaderStageFlagBits::eVertex);


    // disable culling
    std::shared_ptr<VulkanRasterizationState> rasterization =
        VulkanRasterizationStateBuilder()
            .SetCullMode(vk::CullModeFlagBits::eNone)
            .SetDepthBiasEnable(VK_TRUE)
            .build();

    // float bias = max(0.05f * (1.0 - dot(normal, lightDir)), 0.005f);

    // layout
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> setLayouts =
    {
        // SET0 MVP UNIFORM
        m_pDevice->GetDescLayoutPresets().UBO
    };
    m_pPipelineLayout.reset(new VulkanPipelineLayout(m_pDevice, setLayouts));

    // dynamic state depth bias
    std::shared_ptr<VulkanDynamicState> dynamic_state =
        std::make_shared<VulkanDynamicState>(
            std::make_optional(
                std::vector<vk::DynamicState>
                {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor,
                    vk::DynamicState::eDepthBias
                }
            )
        );

    // switch off color blend state
    std::shared_ptr<VulkanColorBlendState> colorBlend = std::make_shared<VulkanColorBlendState>(std::vector<vk::PipelineColorBlendAttachmentState>{});
    auto multisampleState = std::make_shared<VulkanMultisampleState>(vk::SampleCountFlagBits::e1);
    for (int i = 0; i < m_num; i++)
    {
        std::unique_ptr<VulkanRenderPipeline> pipeline =
            VulkanRenderPipelineBuilder(m_pDevice, m_pRenderPasses[i].get())
                .SetVulkanPipelineLayout(m_pPipelineLayout)
                .SetshaderSet(shaderSet)
                .SetVulkanRasterizationState(rasterization)
                .SetVulkanDynamicState(dynamic_state)
                .SetVulkanMultisampleState(multisampleState)
                .buildUnique();
        m_pRenderPasses[i]->AddGraphicRenderPipeline("shadowmap", std::move(pipeline));
    }
}

// No.Light <==> (FRAMES) * No.UniformBuffer <==> (1)UniformBuffer <==> (No.UniformBuffer)UBODescriptorSets
void ShadowMapRenderPass::initUniformBuffer()
{

    m_uniformBuffers.resize(m_num);

    for (int lightId = 0; lightId < m_num; lightId++)
    {
        m_uniformBuffers[lightId].reset(
            new VulkanBuffer(
                m_pDevice, sizeof(CameraUniformBufferObject),
                vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::SharingMode::eExclusive
            ));
    }

}