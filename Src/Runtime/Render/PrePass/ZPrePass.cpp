#include "ZPrePass.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"

using namespace Render;

ZPrePass::ZPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight)
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

ZPrePass::~ZPrePass()
{
    m_pDepthImageSampler.reset();
}

void ZPrePass::prepareLayout()
{
    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice,
            { m_pDevice->GetDescLayoutPresets().UBO }
            , {}
        )
    );
}

void ZPrePass::prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    attachments.resize(1);

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

    // 0: depth attachment
    attachRTConfig.format = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    attachRTConfig.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
    attachRTConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    m_pDepthImageSampler = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[0].resource = m_pDepthImageSampler->GetPImageResource()->GetNative();
    attachments[0].resourceFormat = attachRTConfig.format;
    attachments[0].samples = sampleCount;
    attachments[0].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachments[0].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachments[0].type = RHI::VulkanFramebuffer::kDepthStencil;
}

void ZPrePass::prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice)
                        .SetAttachments(attachments)
                        .SetDefaultSubpass()
                        .AddSubpassDependency(vk::SubpassDependency()
                            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                            .setDstSubpass(0)
                            .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
                            .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests)
                            .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
                            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                            .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                        .AddSubpassDependency(vk::SubpassDependency()
                            .setSrcSubpass(0)
                            .setDstSubpass(VK_SUBPASS_EXTERNAL)
                            .setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
                            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                            .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                            .setDependencyFlags(vk::DependencyFlagBits::eByRegion))
                        .buildUnique();
}

void ZPrePass::prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments)
{
    m_pFramebuffer = std::make_unique<RHI::VulkanFramebuffer>(m_pDevice, m_pRenderPass.get(), m_fbWidth, m_fbHeight, 1, std::move(attachments));
}

void ZPrePass::preparePipeline()
{
    auto sampleCount = vk::SampleCountFlagBits::e1;

    {
        auto shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
        shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/z-prepass.vert.spv", vk::ShaderStageFlagBits::eVertex);
        // shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/mrt.frag.spv", vk::ShaderStageFlagBits::eFragment);
        auto multiSampleState = std::make_shared<RHI::VulkanMultisampleState>(sampleCount);

        auto blendStateAttachment = vk::PipelineColorBlendAttachmentState()
                                        .setColorWriteMask(vk::ColorComponentFlags(0xf))
                                        .setBlendEnable(VK_FALSE);
        auto blendState = std::make_shared<RHI::VulkanColorBlendState>(std::vector<vk::PipelineColorBlendAttachmentState>(1, blendStateAttachment));
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

void ZPrePass::prepareOutputDescriptorSets()
{
    m_pDescriptorPool.reset(new RHI::VulkanDescriptorPool(
        m_pDevice, 
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eCombinedImageSampler, 1 }
        },1));
    m_pDescriptors = m_pDescriptorPool->AllocSamplerDescriptorSet(
        m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER.get(),
        { m_pDepthImageSampler.get() }, {1});
}