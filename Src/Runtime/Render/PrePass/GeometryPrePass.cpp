#include "GeometryPrePass.h"
#include "Runtime/Render/PrePass/PrePass.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>

using namespace Render;


GeometryPrePass::GeometryPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight)
    : PrePass(device, camera)
    , m_fbWidth(fbWidth)
    , m_fbHeight(fbHeight)
{
    prepareLayout();
    {
        std::vector<RHI::VulkanFramebuffer::Attachment> attachments;
        prepareAttachments(attachments);
        prepareRenderPass(attachments);
        prepareFramebuffer(std::move(attachments));
    }
    preparePipeline();
    prepareOutputDescriptorSets();
}

GeometryPrePass::~GeometryPrePass()
{

}


void GeometryPrePass::Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models)
{
    std::vector<vk::ClearValue> clearValues {
        // position
        vk::ClearValue { vk::ClearColorValue { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } } },
        // normal
        vk::ClearValue { vk::ClearColorValue { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f } } },
        // albedo
        vk::ClearValue { vk::ClearColorValue { std::array<float, 4> { 1.0f, 1.0f, 1.0f, 1.0f } } },
        // depth
        vk::ClearValue { vk::ClearDepthStencilValue { 1.0f, 0 } }
    };
    m_pRenderPass->Begin(cmdBuffer, clearValues, vk::Rect2D { vk::Offset2D {0,0}, vk::Extent2D{ m_fbWidth, m_fbHeight } }, m_pFramebuffer->GetVkFramebuffer());
    {
        m_pRenderPass->BindGraphicPipeline(cmdBuffer, "mrt");
        vk::Viewport viewport {0,0,(float)m_fbWidth, (float)m_fbHeight, 0.0f, 1.0f};
        cmdBuffer.setViewport(0,1,&viewport);
        cmdBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0,0}, vk::Extent2D{m_fbWidth, m_fbHeight}});
        std::vector<vk::DescriptorSet> tobinding;
        for (auto model : models)
        {
            model->Draw(cmdBuffer, m_pPipelineLayout.get(), tobinding);
        }
    }
    m_pRenderPass->End(cmdBuffer);
}



void GeometryPrePass::prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    attachments.resize(4);

    auto sampleCount = vk::SampleCountFlagBits::e1;

    RHI::VulkanImageSampler::Config samplerConfig;
    samplerConfig.magFilter = vk::Filter::eNearest;
    samplerConfig.minFilter = vk::Filter::eNearest;
    samplerConfig.uAddressMode = vk::SamplerAddressMode::eClampToEdge;
    samplerConfig.vAddressMode = vk::SamplerAddressMode::eClampToEdge;
    samplerConfig.wAddressMode = vk::SamplerAddressMode::eClampToEdge;
    samplerConfig.borderColor = vk::BorderColor::eFloatOpaqueWhite;


    RHI::VulkanImageResource::Config attachRTConfig;
    attachRTConfig.extent = vk::Extent3D { m_fbWidth, m_fbHeight, 1};
    attachRTConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    attachRTConfig.sampleCount = sampleCount;

    // 0: position attachment samplable
    attachRTConfig.format = vk::Format::eR16G16B16A16Sfloat;
    m_attachmentResources.position = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[0].resource = m_attachmentResources.position->GetPImageResource()->GetNative();
    attachments[0].resourceFormat = attachRTConfig.format;
    attachments[0].samples = sampleCount;
    attachments[0].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachments[0].type = RHI::VulkanFramebuffer::kColor;

    // 1: normal attachment samplable
    RHI::VulkanFramebuffer::Attachment normal;
    attachRTConfig.format = vk::Format::eR16G16B16A16Sfloat;
    m_attachmentResources.normal = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[1].resource = m_attachmentResources.normal->GetPImageResource()->GetNative();
    attachments[1].resourceFormat = attachRTConfig.format;
    attachments[1].samples = sampleCount;
    attachments[1].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachments[1].type = RHI::VulkanFramebuffer::kColor;


    // 2: albedo attachment samplable
    attachRTConfig.format = vk::Format::eR8G8B8A8Unorm;
    m_attachmentResources.albedo = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[2].resource = m_attachmentResources.albedo->GetPImageResource()->GetNative();
    attachments[2].resourceFormat = attachRTConfig.format;
    attachments[2].samples = sampleCount;
    attachments[2].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachments[2].type = RHI::VulkanFramebuffer::kColor;


    // 3: depth attachment
    attachRTConfig.format = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    attachRTConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
    attachRTConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    m_attachmentResources.depth = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[3].resource = m_attachmentResources.depth->GetPImageResource()->GetNative();
    attachments[3].resourceFormat = attachRTConfig.format;
    attachments[3].samples = sampleCount;
    attachments[3].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachments[3].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachments[3].type = RHI::VulkanFramebuffer::kDepthStencil;
}

void GeometryPrePass::prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice)
                        .SetAttachments(attachments)
                        .SetDefaultSubpass()
                        .AddSubpassDependency(vk::SubpassDependency()
                            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                            .setDstSubpass(0)
                            .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                            .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
                            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
                            .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                        .AddSubpassDependency(vk::SubpassDependency()
                            .setSrcSubpass(0)
                            .setDstSubpass(VK_SUBPASS_EXTERNAL)
                            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                            .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
                            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
                            .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                        .buildUnique();
}

void GeometryPrePass::prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments)
{
    m_pFramebuffer = std::make_unique<RHI::VulkanFramebuffer>(m_pDevice, m_pRenderPass.get(), m_fbWidth, m_fbHeight, 1, std::move(attachments));
}

void GeometryPrePass::preparePipeline()
{
    auto sampleCount = vk::SampleCountFlagBits::e1;

    {
        auto shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
        shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/mrt.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/mrt.frag.spv", vk::ShaderStageFlagBits::eFragment);
        auto multiSampleState = std::make_shared<RHI::VulkanMultisampleState>(sampleCount);

        auto blendStateAttachment = vk::PipelineColorBlendAttachmentState()
                                        .setColorWriteMask(vk::ColorComponentFlags(0xf))
                                        .setBlendEnable(VK_FALSE);
        auto blendState = std::make_shared<RHI::VulkanColorBlendState>(std::vector<vk::PipelineColorBlendAttachmentState>(3, blendStateAttachment));
        m_pRenderPass->AddGraphicRenderPipeline(
                        "mrt",
                            RHI::VulkanRenderPipelineBuilder(m_pDevice, m_pRenderPass.get())
                                .SetVulkanPipelineLayout(m_pPipelineLayout)
                                .SetVulkanMultisampleState(multiSampleState)
                                .SetVulkanColorBlendState(blendState)
                                .SetshaderSet(shaderSet)
                                .buildUnique()
                        );
    }
}

void GeometryPrePass::prepareOutputDescriptorSets()
{
    m_pDescriptorPool.reset(new RHI::VulkanDescriptorPool(
        m_pDevice, 
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eCombinedImageSampler, 4 }
        },1));
    m_pDescriptors = m_pDescriptorPool->AllocSamplerDescriptorSet(
        m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER.get(),
        {
            m_attachmentResources.position.get(),
            m_attachmentResources.normal.get(),
            m_attachmentResources.albedo.get(),
            m_attachmentResources.depth.get()
        }, {1,2,3,4});
}