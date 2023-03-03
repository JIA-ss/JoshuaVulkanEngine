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
#include "Runtime/VulkanRHI/PipelineStates/VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
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
    m_vkDepthFormat = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();

    // No.Renderpass <==> (FRAMES) * No.Event
    initSyncPrimitive();

    // No.Light <==> No.RenderPass
    initRenderPass();
    // No.Light <==> (FRAMES) * No.Framebuffer <==> No.DepthSampler
    initDepthSampler();
    initFramebuffer();

    // ALL.DepthSampler <==> (1)DepthSamplerDescriptorSets
    initDepthSamplerDescriptor();
    // No.Light <==> (FRAMES) * No.UniformBuffer <==> (1)UniformBuffer
    initUniformBuffer();
    initPipelines();
}


ShadowMapRenderPass::~ShadowMapRenderPass()
{
    m_pDepthSamplerDescriptorSets.fill(nullptr);
    for (int frameId = 0; frameId < MAX_FRAMES_IN_FLIGHT; frameId++)
    {
        m_pDepthSamplers[frameId].clear();
        m_vkFramebuffers[frameId].clear();
        m_uniformBuffers[frameId].clear();
    }
    m_pDescriptorPool.reset();
    m_pRenderPasses.clear();
}

void ShadowMapRenderPass::InitModelShadowDescriptor(Model* model)
{
    for (int lightId = 0; lightId < m_num; lightId++)
    {
        std::array<std::vector<RHI::Model::UBOLayoutInfo>, MAX_FRAMES_IN_FLIGHT> uboInfos;
        int bindingId = VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID;
        for (int frameId = 0; frameId < uboInfos.size(); frameId++)
        {
            uboInfos[frameId].emplace_back(RHI::Model::UBOLayoutInfo
            {
                m_uniformBuffers[frameId][lightId].get(), VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID, sizeof(CameraUniformBufferObject)
            });
        }
        model->InitShadowPassUniforDescriptorSets(uboInfos, lightId);
    }
}

void ShadowMapRenderPass::SetShadowPassLightVPUBO(CameraUniformBufferObject& ubo, int frameId, int lightIdx)
{
    m_uniformBuffers[frameId][lightIdx]->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
}

void ShadowMapRenderPass::FillDepthSamplerToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout, int frameId)
{
    m_pDepthSamplerDescriptorSets[frameId]->FillToBindedDescriptorSetsVector(descList, pipelineLayout);
}

void ShadowMapRenderPass::Render(vk::CommandBuffer cmd, std::vector<Model*> models, int frameId)
{
    std::vector<vk::ClearValue> clears(1);
    clears[0] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};
    vk::Extent2D extent = vk::Extent2D{m_width, m_height};
    for (int lightIdx = 0; lightIdx < m_num; lightIdx++)
    {
        m_pRenderPasses[lightIdx]->Begin(cmd, clears, vk::Rect2D{vk::Offset2D{0,0}, extent}, m_vkFramebuffers[frameId][lightIdx]);
        {
            m_pRenderPasses[lightIdx]->BindGraphicPipeline(cmd, "shadowmap");
            // cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pPipelineLayout->GetVkPieplineLayout(), 0, tobinding, {});

            vk::Rect2D rect{{0,0},extent};
            cmd.setViewport(0,vk::Viewport{0,0,(float)extent.width, (float)extent.height,0,1});
            cmd.setScissor(0,rect);
            cmd.setDepthBias(DEPTH_BIAS_CONSTANT, 0.0f, DEPTH_BIAS_SLOP);
            for (auto& model : models)
            {
                model->DrawShadowPass(cmd, m_pPipelineLayout.get(), frameId, lightIdx);
            }
        }
        //cmd.setEvent(m_vkEvents[frameId][lightIdx], vk::PipelineStageFlagBits::eEarlyFragmentTests);
        m_pRenderPasses[lightIdx]->End(cmd);
    }


    //cmd.waitEvents(m_vkEvents[frameId], vk::PipelineStageFlagBits::eVertexShader, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {});

}

// No.Renderpass <==> (FRAMES) * No.Event
void ShadowMapRenderPass::initSyncPrimitive()
{
    for (int frameId = 0; frameId < MAX_FRAMES_IN_FLIGHT; ++frameId)
    {
        m_vkEvents[frameId].resize(m_num);
        for (int lightId = 0; lightId < m_num; lightId++)
        {
            vk::EventCreateInfo eventInfo;
            eventInfo.setFlags(vk::EventCreateFlagBits::eDeviceOnlyKHR);
            m_vkEvents[frameId][lightId] = m_pDevice->GetVkDevice().createEvent(eventInfo);
        }
    }
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

    for (int frameId = 0; frameId < m_pDepthSamplers.size(); frameId++)
    {
        m_pDepthSamplers[frameId].resize(m_num);
        for (int lightIdx = 0; lightIdx < m_num; lightIdx++)
        {
            m_pDepthSamplers[frameId][lightIdx].reset(new VulkanImageSampler(
                m_pDevice, nullptr,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                samplerConfig,
                imageConfig)
            );
            // m_pDepthSamplers[frameId]->GetPImageResource()->TransitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
        }
    }
}

// No.Light <==> No.RenderPass
void ShadowMapRenderPass::initRenderPass()
{
    m_pRenderPasses.resize(m_num);
    for (int i = 0; i < m_num; i++)
    {
        std::vector<vk::AttachmentDescription> attachments(1);
        std::vector<vk::SubpassDescription> subpasses(1);
        std::vector<vk::SubpassDependency> subpassDependencies(2);
        std::vector<vk::AttachmentReference> attachRef(1);

        attachments[0]
                    .setFormat(m_vkDepthFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::eDepthStencilReadOnlyOptimal);

        attachRef[0]
                    .setAttachment(0)
                    .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        subpasses[0]
                    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setColorAttachmentCount(0)
                    .setPDepthStencilAttachment(&attachRef[0]);

        subpassDependencies[0]
                    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                    .setDstSubpass(0)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                    .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
                    .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                    .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
        subpassDependencies[1]
                    .setSrcSubpass(0)
                    .setDstSubpass(VK_SUBPASS_EXTERNAL)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
                    .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                    .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                    .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
        vk::RenderPassCreateInfo renderPassCreateInfo;
        renderPassCreateInfo.setAttachments(attachments)
                    .setSubpasses(subpasses)
                    .setDependencies(subpassDependencies);
        m_pRenderPasses[i].reset(new VulkanRenderPass(m_pDevice, m_pDevice->GetVkDevice().createRenderPass(renderPassCreateInfo)));
    }
}

// No.Light <==> (FRAMES) * No.Framebuffer
void ShadowMapRenderPass::initFramebuffer()
{
    for (int frameId = 0; frameId < m_vkFramebuffers.size(); frameId++)
    {
        m_vkFramebuffers[frameId].resize(m_num);
        for (int lightId = 0; lightId < m_num; lightId++)
        {
            auto createInfo = vk::FramebufferCreateInfo()
                        .setRenderPass(m_pRenderPasses[lightId]->GetVkRenderPass())
                        .setAttachments(*m_pDepthSamplers[frameId][lightId]->GetPVkImageView())
                        .setWidth((uint32_t)m_width)
                        .setHeight((uint32_t)m_height)
                        .setLayers(1);
            m_vkFramebuffers[frameId][lightId] = m_pDevice->GetVkDevice().createFramebuffer(createInfo);
        }
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

    for (int frameId = 0; frameId < m_pDepthSamplerDescriptorSets.size(); frameId++)
    {
        std::vector<VulkanImageSampler*> samplers;
        std::vector<uint32_t> binding;
        for (uint32_t bindingId = VulkanDescriptorSetLayout::DESCRIPTOR_SHADOWMAP1_BINDING_ID; bindingId <= VulkanDescriptorSetLayout::DESCRIPTOR_SHADOWMAP5_BINDING_ID; bindingId++)
        {
            samplers.push_back(m_pDepthSamplers[frameId][bindingId < m_num ? bindingId : m_num - 1].get());
            binding.push_back(bindingId);
        }
        m_pDepthSamplerDescriptorSets[frameId] = m_pDescriptorPool->AllocSamplerDescriptorSet(m_pDevice->GetDescLayoutPresets().SHADOWMAP.get(), samplers, binding, vk::ImageLayout::eDepthStencilReadOnlyOptimal, 1);
    }
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
    for (int i = 0; i < m_num; i++)
    {
        std::unique_ptr<VulkanRenderPipeline> pipeline =
            VulkanRenderPipelineBuilder(m_pDevice)
                .SetVulkanPipelineLayout(m_pPipelineLayout)
                .SetshaderSet(shaderSet)
                .SetVulkanRasterizationState(rasterization)
                .SetVulkanRenderPass(m_pRenderPasses[i])
                .SetVulkanDynamicState(dynamic_state)
                .buildUnique();
        m_pRenderPasses[i]->AddGraphicRenderPipeline("shadowmap", std::move(pipeline));
    }
}

// No.Light <==> (FRAMES) * No.UniformBuffer <==> (1)UniformBuffer <==> (No.UniformBuffer)UBODescriptorSets
void ShadowMapRenderPass::initUniformBuffer()
{
    for (int frameId = 0; frameId < m_uniformBuffers.size(); frameId++)
    {
        m_uniformBuffers[frameId].resize(m_num);

        for (int lightId = 0; lightId < m_num; lightId++)
        {
            m_uniformBuffers[frameId][lightId].reset(
                new VulkanBuffer(
                    m_pDevice, sizeof(CameraUniformBufferObject),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
                ));
        }
    }
}