#include "VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "vulkan/vulkan_enums.hpp"
#include <algorithm>

RHI_NAMESPACE_USING

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, vk::Format colorFormat, vk::Format depthFormat, vk::SampleCountFlagBits sample)
    : m_vulkanDevice(device)
    , m_colorFormat(colorFormat)
    , m_depthFormat(depthFormat)
{
    bool usingMSAA = sample > vk::SampleCountFlagBits::e1;

    vk::AttachmentDescription colorAttach, depthAttach, colorAttachResolve;
    vk::AttachmentReference colorAttachRef, depthAttachRef, colorAttachResolveRef;
    colorAttach.setFormat(colorFormat)
                .setSamples(sample)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(usingMSAA ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR);

    depthAttach.setFormat(depthFormat)
                .setSamples(sample)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    uint32_t attachmentIdx = 0;

    colorAttachRef.setAttachment(attachmentIdx++)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    depthAttachRef.setAttachment(attachmentIdx++)
                .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);


    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachments(colorAttachRef)
                .setPDepthStencilAttachment(&depthAttachRef)
                ;

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                .setSrcAccessMask(vk::AccessFlagBits(0))
                .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::vector<vk::AttachmentDescription> attachments {colorAttach, depthAttach};
    if (usingMSAA)
    {
        colorAttachResolve.setFormat(colorFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        colorAttachResolveRef.setAttachment(attachmentIdx++)
                    .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        subpass.setResolveAttachments(colorAttachResolveRef);
        attachments.push_back(colorAttachResolve);
    }



    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.setAttachments(attachments)
                .setSubpasses(subpass)
                .setDependencies(dependency);

    m_vkRenderPass = m_vulkanDevice->GetVkDevice().createRenderPass(renderPassCreateInfo);
}


VulkanRenderPass::~VulkanRenderPass()
{
    m_vulkanDevice->GetVkDevice().destroyRenderPass(m_vkRenderPass);
}

void VulkanRenderPass::Begin(vk::CommandBuffer cmd, const std::vector<vk::ClearValue>& clearValues, const vk::Rect2D& renderArea, vk::Framebuffer frameBuffer, vk::SubpassContents contents)
{
    auto renderpassBeginInfo = vk::RenderPassBeginInfo()
                            .setRenderPass(GetVkRenderPass())
                            .setClearValues(clearValues)
                            .setRenderArea(renderArea)
                            .setFramebuffer(frameBuffer);
    cmd.beginRenderPass(renderpassBeginInfo , contents);
}

void VulkanRenderPass::End(vk::CommandBuffer cmd)
{
    cmd.endRenderPass();
}

void VulkanRenderPass::BindGraphicPipeline(vk::CommandBuffer cmd, const std::string& name)
{
    VulkanRenderPipeline* pipeline = GetGraphicRenderPipeline(name);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->GetVkPipeline());
}

void VulkanRenderPass::AddGraphicRenderPipeline(const std::string& name, std::unique_ptr<VulkanRenderPipeline> pipeline)
{
    for (auto& pipeline : m_graphicPipelines)
    {
        assert(pipeline.name != name);
    }
    m_graphicPipelines.emplace_back(VulkanPipelineWrapper{std::move(pipeline), name});
}

VulkanRenderPipeline* VulkanRenderPass::GetGraphicRenderPipeline(const std::string& name)
{
    for (auto& pipeline : m_graphicPipelines)
    {
        if(pipeline.name == name)
        {
            return pipeline.pipeline.get();
        }
    }
    assert(false);
    return nullptr;
}
