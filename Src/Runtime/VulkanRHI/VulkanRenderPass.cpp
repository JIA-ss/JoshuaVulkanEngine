#include "VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"

RHI_NAMESPACE_USING

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, vk::Format colorFormat, vk::Format depthFormat, vk::SampleCountFlagBits sample)
    : m_vulkanDevice(device)
    , m_colorFormat(colorFormat)
    , m_depthFormat(depthFormat)
{
    vk::AttachmentDescription colorAttach, depthAttach, colorAttachResolve;
    colorAttach.setFormat(colorFormat)
                .setSamples(sample)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    depthAttach.setFormat(depthFormat)
                .setSamples(sample)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    // colorAttachResolve.setFormat(colorFormat)
    //             .setSamples(sample)
    //             .setLoadOp(vk::AttachmentLoadOp::eDontCare)
    //             .setStoreOp(vk::AttachmentStoreOp::eStore)
    //             .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
    //             .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
    //             .setInitialLayout(vk::ImageLayout::eUndefined)
    //             .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachRef, depthAttachRef, colorAttachResolveRef;
    colorAttachRef.setAttachment(0)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    depthAttachRef.setAttachment(1)
                .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    // colorAttachResolveRef.setAttachment(2)
    //             .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachments(colorAttachRef)
                .setPDepthStencilAttachment(&depthAttachRef)
                ;//.setResolveAttachments(colorAttachResolveRef);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                .setSrcAccessMask(vk::AccessFlagBits(0))
                .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    // std::array<vk::AttachmentDescription, 3> attachments {colorAttach, depthAttach, colorAttachResolve};
    std::array<vk::AttachmentDescription, 2> attachments {colorAttach, depthAttach};// colorAttachResolve};
    // std::array<vk::AttachmentDescription, 1> attachments {colorAttach};

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