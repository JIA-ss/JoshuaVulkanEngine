#include "PrePass.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

using namespace Render;

PrePass::PrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight)
    : m_pDevice(device)
    , m_pCamera(camera)
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
}

PrePass::~PrePass()
{

}


void PrePass::prepareLayout()
{

}

void PrePass::prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
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
    attachRTConfig.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    attachRTConfig.sampleCount = sampleCount;

    // 0: position attachment samplable
    attachRTConfig.format = vk::Format::eR16G16B16A16Sfloat;
    m_attachmentResources.position = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[0].resource = m_attachmentResources.position->GetPImageResource()->GetNative();
    attachments[0].samples = sampleCount;
    attachments[0].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;

    // 1: normal attachment samplable
    RHI::VulkanFramebuffer::Attachment normal;
    attachRTConfig.format = vk::Format::eR16G16B16A16Sfloat;
    m_attachmentResources.normal = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[1].resource = m_attachmentResources.normal->GetPImageResource()->GetNative();
    attachments[1].samples = sampleCount;
    attachments[1].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;


    // 2: albedo attachment samplable
    attachRTConfig.format = vk::Format::eR8G8B8A8Unorm;
    m_attachmentResources.albedo = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[2].resource = m_attachmentResources.albedo->GetPImageResource()->GetNative();
    attachments[2].samples = sampleCount;
    attachments[2].attachmentReferenceLayout = vk::ImageLayout::eColorAttachmentOptimal;


    // 3: depth attachment
    attachRTConfig.format = m_pDevice->GetVulkanPhysicalDevice()->QuerySupportedDepthFormat();
    attachRTConfig.imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    m_attachmentResources.depth = std::make_unique<RHI::VulkanImageSampler>(m_pDevice, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal, samplerConfig,  attachRTConfig);
    attachments[3].resource = m_attachmentResources.depth->GetPImageResource()->GetNative();
    attachments[3].samples = sampleCount;
    attachments[3].resourceFinalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    attachments[3].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

void PrePass::prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    std::vector<vk::AttachmentDescription> desc(attachments.size());
    std::vector<vk::AttachmentReference> colorAttachRefs;
    vk::AttachmentReference depthAttachRef;
    for (int i = 0; i < attachments.size(); i++)
    {
        desc[i] = attachments[i].GetVkAttachmentDescription();
        auto attachmentReferenceLayout = attachments[i].attachmentReferenceLayout;
        auto attachmentReference = attachments[i].GetVkAttachmentReference(i);
        if (attachmentReferenceLayout == vk::ImageLayout::eColorAttachmentOptimal)
        {
            colorAttachRefs.push_back(attachmentReference);
        }
        else if (attachmentReferenceLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            depthAttachRef = attachmentReference;
        }
    }

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachRefs)
            .setPDepthStencilAttachment(&depthAttachRef)
            ;

    std::array<vk::SubpassDependency, 2> dependencies;
    dependencies[0]
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
        .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
        ;
    dependencies[1]
        .setSrcSubpass(0)
        .setDstSubpass(VK_SUBPASS_EXTERNAL)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
        .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
        ;

    vk::RenderPassCreateInfo rpCI;
    rpCI
        .setAttachments(desc)
        .setSubpasses(subpass)
        .setDependencies(dependencies)
        ;
    auto vkRp = m_pDevice->GetVkDevice().createRenderPass(rpCI);
    m_pRenderPass = std::make_unique<RHI::VulkanRenderPass>(m_pDevice, vkRp);
}

void PrePass::prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments)
{
    m_pFramebuffer = std::make_unique<RHI::VulkanFramebuffer>(m_pDevice, m_pRenderPass.get(), m_fbWidth, m_fbHeight, 1, std::move(attachments));
}

void PrePass::preparePipeline()
{

}