#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "Runtime/RenderGraph/RenderDependencyGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/RenderGraph/Pass/RenderGraphPassNode.h"
#include "Runtime/RenderGraph/Resource/RenderGraphVirtualResource.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
using namespace Render;

/*
eg.
rg.addPassNode("Present Pass", [&](RenderGraphBuild& build)
{
    auto& outputTexData = scope.get<RenderGraphOutputTexture>();
    auto finalTexture = build.read(outputTexData.texture, RenderGraphResourceFlags::ShaderResource);
    auto outputTexture = build.write(outputTexData.texture, RenderGraphResourceFlags::Present);
    return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList)
    {
        auto& finalTextureData = registry.get<RenderGraphTexture>(finalTexture);
        auto& outputTextureData = registry.get<RenderGraphTexture>(outputTexture);
        cmdList.copyTexture(finalTextureData.texture, outputTextureData.texture);
    };
});
*/

RenderGraph::RenderGraph(RHI::VulkanDevice* device) : m_device(device)
{
    auto native = m_device->GetPVulkanSwapchain()->GetVulkanPresentImage(0);
    RenderGraphBuilder builder(this, nullptr);
    m_presentHandle = builder.createImageResourceNode("Present Image", m_device, native);

    // avoid release present image
    m_resourceNodes[m_presentHandle]->incRefCount();
}

RenderGraph::~RenderGraph()
{
    RenderGraphRegistry registry(this, nullptr);
    auto virtualResource = registry.getVirtualResource(m_presentHandle);
    virtualResource->getResourceRef()->getInternalResource()->SetNative({});

    destroySyncObjects();
}

void RenderGraph::compile()
{
    m_compiledRenderPasses.clear();

    cullNodes();
    mergePass();
    compileDiscriptorsets();
    createSyncObjects();
}

void RenderGraph::execute()
{
    if (auto command = prepareCommand(); command.has_value())
    {
        for (auto& compiledPass : m_compiledRenderPasses)
        {
            excuteCompiledPass(compiledPass, command.value());
        }
        finishCommand(command.value());
    }
}

void RenderGraph::cullNodes()
{
    // cull isolate nodes
    auto culledNodes = m_dependencyGraph.cullIsolateNodes();
    // release unused resource
    for (auto Node : culledNodes)
    {
        if (auto resourceNode = dynamic_cast<RenderGraphResourceNode*>(Node); resourceNode)
        {
            auto handle = resourceNode->getHandle();
            auto& virtualResource = m_virtualResources[handle.getId()];
            assert(!virtualResource->isReleased() && "resource is already released");
            virtualResource->releaseResource();
            std::cout << "release resource: " << resourceNode->getName() << std::endl;
        }
    }
}

void RenderGraph::mergePass()
{
    // get first valid pass node;
    size_t headPassIndex = 0;
    for (int i = 0; i < m_passNodes.size(); ++i)
    {
        if (!m_dependencyGraph.isIsolate(m_passNodes[i].get()))
        {
            headPassIndex = i;
            break;
        }
    }

    auto getAttachmentDescriptions =
        [](const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) -> std::vector<vk::AttachmentDescription>
    {
        std::vector<vk::AttachmentDescription> result;
        std::transform(
            attachments.begin(), attachments.end(), std::back_inserter(result),
            [](const RHI::VulkanFramebuffer::Attachment& attachment)
            { return attachment.GetVkAttachmentDescription(); });
        return result;
    };

    auto isAttachmentDescriptionsEqual =
        [getAttachmentDescriptions](
            const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments1,
            const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments2) -> bool
    {
        auto desc1 = getAttachmentDescriptions(attachments1);
        auto desc2 = getAttachmentDescriptions(attachments2);
        if (desc1.size() != desc2.size())
        {
            return false;
        }

        for (int i = 0; i < desc1.size(); i++)
        {
            if (desc1[i] != desc2[i])
            {
                return false;
            }
        }
        return true;
    };

    auto bindingInfo = m_passNodes[headPassIndex]->getBindingInfo();
    auto attachments = m_passNodes[headPassIndex]->getAttachments();
    auto attachmentMeta = m_passNodes[headPassIndex]->getAttachmentMeta();

    for (int i = headPassIndex + 1; i < m_passNodes.size(); i++)
    {
        // merge binding info
        auto mergedBindingInfo = bindingInfo.Merge(m_passNodes[i]->getBindingInfo());

        // currentAttachmentMeta
        auto currentAttachmentMeta = m_passNodes[i]->getAttachmentMeta();

        // merge failed
        if (!mergedBindingInfo.has_value() || currentAttachmentMeta != attachmentMeta ||
            !isAttachmentDescriptionsEqual(attachments, m_passNodes[i]->getAttachments()) ||
            i == m_passNodes.size() - 1)
        {
            m_compiledRenderPasses.emplace_back(CompiledRenderPass{});
            compileRenderPass(headPassIndex, i - 1, bindingInfo);
            headPassIndex = i;
            bindingInfo = m_passNodes[i]->getBindingInfo();
            continue;
        }

        bindingInfo = mergedBindingInfo.value();
    }
}

void RenderGraph::compileDiscriptorsets()
{
    for (auto& compiledPass : m_compiledRenderPasses)
    {
        RHI::VulkanDelayDescriptorPool delayPool(m_device);
        auto setNum = compiledPass.bindingInfo.setBindings.size();

        for (int setId = 0; setId < setNum; setId++)
        {

            int validPassIdx = 0;
            for (int passId = compiledPass.headPassIndex; passId <= compiledPass.tailPassIndex; passId++)
            {
                auto node = m_passNodes[passId].get();
                if (m_dependencyGraph.isIsolate(node))
                {
                    continue;
                }
                auto readReosurceNodes = m_dependencyGraph.getSourceNodes(node);
                for (auto& readNode : readReosurceNodes)
                {
                    auto resourceNode = static_cast<RenderGraphResourceNode*>(readNode);
                    auto description = resourceNode->getVirtualResource()->getDescription();
                    if (description.bufferRange.has_value())
                    {
                        if (auto bufferResource = dynamic_cast<RenderGraphVirtualResource<RHI::VulkanBuffer>*>(
                                resourceNode->getVirtualResource());
                            bufferResource)
                        {
                            bufferInfos.push_back(
                                vk::DescriptorBufferInfo()
                                    .setBuffer(*bufferResource->getResourceRef()->getInternalResource()->GetPVkBuf())
                                    .setOffset(vk::DeviceSize{description.bufferOffset.value()})
                                    .setRange(vk::DeviceSize{description.bufferRange.value()}));
                            writeDescs.back().setBufferInfo(bufferInfos.back());
                        }
                        else
                        {
                            throw std::runtime_error("buffer resource not found");
                        }
                    }
                    else
                    {
                        if (auto samplerResource = dynamic_cast<RenderGraphVirtualResource<RHI::VulkanImageSampler>*>(
                                resourceNode->getVirtualResource());
                            samplerResource)
                        {
                            imageInfos.push_back(
                                vk::DescriptorImageInfo()
                                    .setImageLayout(samplerResource->getResourceRef()
                                                        ->getInternalResource()
                                                        ->GetConfig()
                                                        .imageLayout)
                                    .setImageView(
                                        *samplerResource->getResourceRef()->getInternalResource()->GetPVkImageView())
                                    .setSampler(
                                        *samplerResource->getResourceRef()->getInternalResource()->GetPVkSampler()));
                            writeDescs.back().setImageInfo(imageInfos.back());
                        }
                        else
                        {
                            throw std::runtime_error("image resource not found");
                        }
                    }
                }

                validPassIdx++;
            }

            m_device->GetVkDevice().updateDescriptorSets((uint32_t)writeDescs.size(), writeDescs.data(), 0, nullptr);
        }
    }
}

void RenderGraph::createDiscriptorPool(CompiledRenderPass& compiledPass)
{
    uint32_t validPassNum = getValidPassNum(compiledPass.headPassIndex, compiledPass.tailPassIndex);
    RHI::VulkanDelayDescriptorPool delayPool(m_device, );
    auto setNum = compiledPass.bindingInfo.setBindings.size();
    uint32_t maxSets = validPassNum * setNum;

    std::map<vk::DescriptorType, uint32_t> descriptorTypeCount;
    for (auto& set : compiledPass.bindingInfo.setBindings)
    {
        for (auto& binding : set)
        {
            descriptorTypeCount[binding.descriptorType] += binding.descriptorCount;
        }
    }

    std::vector<vk::DescriptorPoolSize> poolSize;
    for (auto& [type, count] : descriptorTypeCount)
    {
        poolSize.push_back(vk::DescriptorPoolSize{type, count});
    }

    compiledPass.m_descriptorPool = std::make_unique<RHI::VulkanDescriptorPool>(m_device, poolSize, maxSets);
}

int RenderGraph::getValidPassNum(int headIndex, int tailIndex)
{
    int validPassNum = 0;
    for (auto passId = headIndex; passId <= tailIndex; passId++)
    {
        if (!m_dependencyGraph.isIsolate(m_passNodes[passId].get()))
        {
            validPassNum++;
        }
    }
    return validPassNum;
}

void RenderGraph::compileRenderPass(
    int headIndex, int tailIndex, const RHI::VulkanPipelineLayout::BindingInfo& bindingInfo)
{
    assert(headIndex >= 0 && headIndex < m_passNodes.size());
    assert(tailIndex >= 0 && tailIndex < m_passNodes.size());
    assert(headIndex <= tailIndex);

    m_compiledRenderPasses.resize(m_compiledRenderPasses.size() + 1);

    auto& compiledRenderPass = m_compiledRenderPasses.back();
    compiledRenderPass.headPassIndex = headIndex;
    compiledRenderPass.tailPassIndex = tailIndex;
    compiledRenderPass.bindingInfo = bindingInfo;

    auto attachments = m_passNodes[headIndex]->getAttachments();
    auto& attachmentMeta = m_passNodes[headIndex]->getAttachmentMeta();

    createLayout(compiledRenderPass);
    createRenderpass(compiledRenderPass, attachments);
    createRenderPipeline(compiledRenderPass);

    auto isAttachmentResourceEqual = [](const std::vector<RHI::VulkanFramebuffer::Attachment>& attachment1,
                                        const std::vector<RHI::VulkanFramebuffer::Attachment>& attachment2) -> bool
    {
        if (attachment1.size() != attachment2.size())
        {
            return false;
        }
        for (int i = 0; i < attachment1.size(); i++)
        {
            if (attachment1[i].resource.vkImage != attachment2[i].resource.vkImage ||
                attachment1[i].resource.vkImageView != attachment2[i].resource.vkImageView)
            {
                return false;
            }
        }
        return true;
    };

    compiledRenderPass.compiledFramebuffer = std::make_unique<CompileFramebuffer>();
    compileFramebuffer(
        compiledRenderPass, attachments, attachmentMeta.width, attachmentMeta.height, attachmentMeta.layer,
        attachmentMeta.fullScreen);
    compiledRenderPass.passIdxToFbIdx[headIndex] = compiledRenderPass.framebuffers.size() - 1;

    for (int i = headIndex + 1; i <= tailIndex; i++)
    {
        auto& passNode = m_passNodes[i];
        auto& passNodeAttachments = passNode->getAttachments();

        if (isAttachmentResourceEqual(attachments, passNodeAttachments))
        {
            compiledRenderPass.passIdxToFbIdx[i] = compiledRenderPass.framebuffers.size() - 1;
            continue;
        }

        compileFramebuffer(
            compiledRenderPass, passNodeAttachments, attachmentMeta.width, attachmentMeta.height, attachmentMeta.layer,
            attachmentMeta.fullScreen);
        compiledRenderPass.passIdxToFbIdx[i] = compiledRenderPass.framebuffers.size() - 1;
        attachments = passNodeAttachments;
    }

    int presentFbNum = 0;
    for (auto& [passIdx, fbIdx] : compiledRenderPass.passIdxToFbIdx)
    {
        if (compiledRenderPass.framebuffers[fbIdx] == nullptr)
        {
            presentFbNum++;
            std::cout << m_passNodes[passIdx]->getName() << ": is present pass" << std::endl;
            assert(presentFbNum == 1 && "present framebuffer should be most one");
        }
    }
    assert(presentFbNum == 1 && "present framebuffer should be at least one");
}

void RenderGraph::updatePassNodeAttachments(
    RenderGraphPassNode* passNode, const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    auto passNodeAttachments = passNode->getAttachments();
    assert(passNodeAttachments.size() <= attachments.size());
    for (int i = 0; i < passNodeAttachments.size(); i++)
    {
        auto native = passNodeAttachments[i].resource;
        passNodeAttachments[i] = attachments[i];
        passNodeAttachments[i].resource = native;
    }
    passNode->setAttachments(passNodeAttachments);
}

void RenderGraph::createLayout(CompiledRenderPass& compiledRenderPass)
{
    compiledRenderPass.pipelineLayout =
        std::make_shared<RHI::VulkanPipelineLayout>(m_device, compiledRenderPass.bindingInfo);
}

void RenderGraph::createRenderpass(
    CompiledRenderPass& compiledRenderPass, const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    assert(compiledRenderPass.pipelineLayout && "pipeline layout is null");

    RHI::VulkanRenderPassBuilder rpBuilder(m_device);
    rpBuilder.SetAttachments(attachments);

    for (int passIdx = compiledRenderPass.headPassIndex; passIdx <= compiledRenderPass.tailPassIndex; passIdx++)
    {
        rpBuilder.AddSubpass(m_passNodes[passIdx]->getSubpassDescription());
        auto dependency = m_passNodes[passIdx]->getSubpassDependencies();
        dependency[0]
            .setSrcSubpass(passIdx == compiledRenderPass.headPassIndex ? VK_SUBPASS_EXTERNAL : passIdx - 1)
            .setDstSubpass(passIdx);
        dependency[1].setSrcSubpass(passIdx).setDstSubpass(
            passIdx == compiledRenderPass.tailPassIndex ? VK_SUBPASS_EXTERNAL : passIdx + 1);
        rpBuilder.AddSubpassDependency(dependency[0]);
        rpBuilder.AddSubpassDependency(dependency[1]);
    }

    compiledRenderPass.renderPass = rpBuilder.buildUnique();
}

void RenderGraph::createRenderPipeline(CompiledRenderPass& compiledRenderPass)
{
    assert(compiledRenderPass.pipelineLayout && "pipeline layout is null");
    assert(compiledRenderPass.renderPass && "render pass is null");
    for (int passId = compiledRenderPass.headPassIndex; passId <= compiledRenderPass.tailPassIndex; passId++)
    {
        auto& passNode = m_passNodes[passId];
        auto& pipelineBuilder = passNode->getPipelineBuilder();
        pipelineBuilder.SetVulkanPipelineLayout(compiledRenderPass.pipelineLayout)
            .SetRenderPass(compiledRenderPass.renderPass.get());

        compiledRenderPass.renderPass->AddGraphicRenderPipeline(passNode->getName(), pipelineBuilder.buildUnique());
    }
}

void RenderGraph::compileFramebuffer(
    RenderGraphPassNode* passNode, CompiledRenderPass& compiledRenderPass,
    const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments, uint32_t attachmentWidth,
    uint32_t attachmentHeight, uint32_t attachmentLayers, bool fullscreen)
{
    if (fullscreen)
    {
        attachmentWidth = m_device->GetSwapchainExtent().width;
        attachmentHeight = m_device->GetSwapchainExtent().height;
    }

    int presentImageId = -1;
    for (int attId = 0; attId < attachments.size(); attId++)
    {
        if (attachments[attId].resourceFinalLayout == vk::ImageLayout::ePresentSrcKHR)
        {
            presentImageId = attId;
            break;
        }
    }

    if (presentImageId != -1)
    {
        compiledRenderPass.framebuffers.push_back(nullptr);
        m_device->CreateVulkanPresentFramebuffer(
            compiledRenderPass.renderPass.get(), attachmentWidth, attachmentHeight, presentImageId);
        return;
    }

    compiledRenderPass.framebuffers.push_back(std::make_unique<RHI::VulkanFramebuffer>(
        m_device, compiledRenderPass.renderPass.get(), attachmentWidth, attachmentHeight, attachmentLayers,
        attachments));
}

void RenderGraph::createSyncObjects()
{
    auto frames = m_device->GetPVulkanSwapchain()->GetVulkanPresentImages().size();
    m_syncObjects.cmds.resize(frames);
    m_syncObjects.fences.resize(frames);
    m_syncObjects.semaphoreImageAvaliables.resize(frames);
    m_syncObjects.semaphoreRenderFinisheds.resize(frames);
    for (int i = 0; i < frames; ++i)
    {
        m_syncObjects.cmds[i] = m_device->GetPVulkanCmdPool()->CreateReUsableCmd();

        vk::FenceCreateInfo fenceInfo;
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
        m_syncObjects.fences[i] = m_device->GetVkDevice().createFence(fenceInfo);

        vk::SemaphoreCreateInfo semaphoreInfo;
        m_syncObjects.semaphoreImageAvaliables[i] = m_device->GetVkDevice().createSemaphore(semaphoreInfo);
        m_syncObjects.semaphoreRenderFinisheds[i] = m_device->GetVkDevice().createSemaphore(semaphoreInfo);
    }
}

void RenderGraph::destroySyncObjects()
{
    for (int i = 0; i < m_syncObjects.cmds.size(); ++i)
    {
        m_device->GetPVulkanCmdPool()->FreeReUsableCmd(m_syncObjects.cmds[i]);
        m_syncObjects.cmds[i] = nullptr;
        auto res =
            m_device->GetVkDevice().waitForFences(m_syncObjects.fences[i], true, std::numeric_limits<uint64_t>::max());
        assert(res == vk::Result::eSuccess);
        m_device->GetVkDevice().destroyFence(m_syncObjects.fences[i]);
        m_device->GetVkDevice().destroySemaphore(m_syncObjects.semaphoreImageAvaliables[i]);
        m_device->GetVkDevice().destroySemaphore(m_syncObjects.semaphoreRenderFinisheds[i]);
        m_syncObjects.semaphoreImageAvaliables[i] = nullptr;
        m_syncObjects.semaphoreRenderFinisheds[i] = nullptr;
        m_syncObjects.fences[i] = nullptr;
    }
}

void RenderGraph::recreateSwapchain() {}

std::optional<vk::CommandBuffer*> RenderGraph::prepareCommand()
{
    if (m_device->GetVkDevice().waitForFences(
            m_syncObjects.fences[m_currentFrameIndex], true, std::numeric_limits<uint64_t>::max()) !=
        vk::Result::eSuccess)
    {
        throw std::runtime_error("wait for inflight fence failed");
    }

    // acquire image
    vk::Result acquireImageResult;
    try
    {
        auto res = m_device->GetVkDevice().acquireNextImageKHR(
            m_device->GetPVulkanSwapchain()->GetSwapchain(), std::numeric_limits<uint64_t>::max(),
            m_syncObjects.semaphoreImageAvaliables[m_currentFrameIndex], nullptr);
        acquireImageResult = res.result;
        m_imageIdx = res.value;
        if (res.result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapchain();
            return std::nullopt;
        }
    }
    catch (vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (m_frameBufferSizeChanged || acquireImageResult == vk::Result::eErrorOutOfDateKHR)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
        return std::nullopt;
    }

    if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("acquire next image failed");
    }

    // reset fence after acquiring the image
    m_device->GetVkDevice().resetFences(m_syncObjects.fences[m_currentFrameIndex]);

    // record command buffer
    m_syncObjects.cmds[m_currentFrameIndex].reset();
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_syncObjects.cmds[m_currentFrameIndex].begin(beginInfo);
    return &m_syncObjects.cmds[m_currentFrameIndex];
}

void RenderGraph::finishCommand(vk::CommandBuffer* cmd)
{
    cmd->end();

    // submit
    std::array<vk::PipelineStageFlags, 1> waitStage{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    auto submitInfo = vk::SubmitInfo()
                          .setWaitSemaphores(m_syncObjects.semaphoreImageAvaliables[m_frameBufferSizeChanged])
                          .setWaitDstStageMask(waitStage)
                          .setCommandBuffers(*cmd)
                          .setSignalSemaphores(m_syncObjects.semaphoreRenderFinisheds[m_frameBufferSizeChanged]);
    m_device->GetVkGraphicQueue().submit(submitInfo, m_syncObjects.fences[m_frameBufferSizeChanged]);

    // present
    auto presentInfo = vk::PresentInfoKHR()
                           .setImageIndices(m_imageIdx)
                           .setSwapchains(m_device->GetPVulkanSwapchain()->GetSwapchain())
                           .setWaitSemaphores(m_syncObjects.semaphoreRenderFinisheds[m_frameBufferSizeChanged]);
    vk::Result presentRes;
    try
    {
        presentRes = m_device->GetVkPresentQueue().presentKHR(presentInfo);
    }
    catch (vk::OutOfDateKHRError)
    {
        m_frameBufferSizeChanged = true;
    }

    if (presentRes == vk::Result::eErrorOutOfDateKHR || presentRes == vk::Result::eSuboptimalKHR ||
        m_frameBufferSizeChanged)
    {
        m_frameBufferSizeChanged = false;
        recreateSwapchain();
    }
    else if (presentRes != vk::Result::eSuccess)
    {
        throw std::runtime_error("present image failed");
    }
}

void RenderGraph::excuteCompiledPass(CompiledRenderPass& compiledRenderPass, vk::CommandBuffer* cmd)
{
    auto& headPassNode = m_passNodes[compiledRenderPass.headPassIndex];
    auto& attmeta = headPassNode->getAttachmentMeta();

    vk::Rect2D renderArea{
        {},
        {attmeta.fullScreen ? m_device->GetSwapchainExtent().width : attmeta.width,
         attmeta.fullScreen ? m_device->GetSwapchainExtent().height : attmeta.height}};
    for (auto& [passIdx, fbIdx] : compiledRenderPass.passIdxToFbIdx)
    {
        compiledRenderPass.renderPass->Begin(
            *cmd, attmeta.clearValues, renderArea, compiledRenderPass.framebuffers[fbIdx]->GetVkFramebuffer());
        auto& passNode = m_passNodes[passIdx];
        RenderGraphRegistry registry(this, passNode.get());
        passNode->getExecutor()->execute(registry, cmd);
        compiledRenderPass.renderPass->End(*cmd);
    }
}
