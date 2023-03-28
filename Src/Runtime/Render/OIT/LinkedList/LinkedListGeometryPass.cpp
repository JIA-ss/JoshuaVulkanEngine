#include "LinkedListGeometryPass.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/PipelineStates/VulkanMultisampleState.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRenderPipeline.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>

using namespace Render;

LinkedListGeometryPass::LinkedListGeometryPass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight, RHI::VulkanImageResource* depthAttachmentResource)
    : PrePass(device, camera)
    , m_fbWidth(fbWidth)
    , m_fbHeight(fbHeight)
    , m_pDepthAttachmentResource(depthAttachmentResource)
{
    prepareLayout();
    {
        std::vector<RHI::VulkanFramebuffer::Attachment> attachments;
        prepareAttachments(attachments);
        prepareRenderPass(attachments);
        prepareFramebuffer(std::move(attachments));
    }
    prepareSBO();
    preparePipeline();
    prepareOutputDescriptorSets();
}

LinkedListGeometryPass::~LinkedListGeometryPass()
{
    m_linkedlistSBOGPUData.linkedListBuffer.reset();
    m_linkedlistSBOGPUData.headIndexImageResource.reset();
    m_linkedlistSBOGPUData.metaBuffer.reset();
}

void LinkedListGeometryPass::Render(vk::CommandBuffer &cmdBuffer, const std::vector<RHI::Model *> &models)
{
    ZoneScopedN("LinkedListGeometryPass::Render");
    std::vector<vk::ClearValue> clears(1);
    clears[0] = vk::ClearValue {vk::ClearDepthStencilValue{1.0f, 0}};

    vk::ClearColorValue cv;
    cv.uint32[0] = 0xffffffff;
    cmdBuffer.clearColorImage(m_linkedlistSBOGPUData.headIndexImageResource->GetVkImage(), vk::ImageLayout::eGeneral,cv,
        vk::ImageSubresourceRange()
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setLevelCount(1)
                    .setLayerCount(1));
    cmdBuffer.fillBuffer(*m_linkedlistSBOGPUData.metaBuffer->GetPVkBuf(), 0, sizeof(uint32_t), 0);

    vk::MemoryBarrier enterBarrier;
    enterBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(), enterBarrier, nullptr, nullptr);

    m_pRenderPass->Begin(cmdBuffer, clears, vk::Rect2D{vk::Offset2D{0,0}, vk::Extent2D{m_fbWidth, m_fbHeight}}, m_pFramebuffer->GetVkFramebuffer());
    {
        vk::Rect2D rect{{0,0},vk::Extent2D{m_fbWidth, m_fbHeight}};
        cmdBuffer.setViewport(0,vk::Viewport{0,0,(float)m_fbWidth, (float)m_fbHeight,0,1});
        cmdBuffer.setScissor(0,rect);
        m_pRenderPass->BindGraphicPipeline(cmdBuffer, "geometry");
        std::vector<vk::DescriptorSet> tobinding;
        tobinding.resize(3);
        tobinding[2] = m_pDescriptors->GetVkDescriptorSet(0);
        for (auto model : models)
        {
            model->Draw(cmdBuffer, m_pPipelineLayout.get(), tobinding);
        }
    }
    m_pRenderPass->End(cmdBuffer);

    vk::MemoryBarrier exitBarrier;
    exitBarrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(), exitBarrier, nullptr, nullptr);
}

void LinkedListGeometryPass::prepareLayout()
{
    m_pLinkedListDescriptorSetLayout = std::make_shared<RHI::VulkanDescriptorSetLayout>(m_pDevice);

    // {
    //     // layout: UBO binding [0-2]
    //     m_pLinkedListDescriptorSetLayout->AddBinding(
    //         RHI::VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID,
    //         vk::DescriptorSetLayoutBinding()
    //             .setBinding(RHI::VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID)
    //             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    //             .setDescriptorCount(1)
    //             .setStageFlags(vk::ShaderStageFlagBits::eVertex)
    //     );
    //     m_pLinkedListDescriptorSetLayout->AddBinding(
    //         RHI::VulkanDescriptorSetLayout::DESCRIPTOR_LIGHTUBO_BINDING_ID,
    //         vk::DescriptorSetLayoutBinding()
    //             .setBinding(RHI::VulkanDescriptorSetLayout::DESCRIPTOR_LIGHTUBO_BINDING_ID)
    //             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    //             .setDescriptorCount(1)
    //             .setStageFlags(vk::ShaderStageFlagBits::eVertex)
    //     );
    //     m_pLinkedListDescriptorSetLayout->AddBinding(
    //         RHI::VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID,
    //         vk::DescriptorSetLayoutBinding()
    //             .setBinding(RHI::VulkanDescriptorSetLayout::DESCRIPTOR_MODELUBO_BINDING_ID)
    //             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    //             .setDescriptorCount(1)
    //             .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
    //     );
    // }

    {
        // layout: SBO binding [0-1]
        m_pLinkedListDescriptorSetLayout->AddBinding(
            0,
            vk::DescriptorSetLayoutBinding()
                .setBinding(0)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        );
        m_pLinkedListDescriptorSetLayout->AddBinding(
            1,
            vk::DescriptorSetLayoutBinding()
                .setBinding(1)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        );
    }

    {
        // layout: uimage2D binding [2]
        m_pLinkedListDescriptorSetLayout->AddBinding(
            2,
            vk::DescriptorSetLayoutBinding()
                .setBinding(2)
                .setDescriptorType(vk::DescriptorType::eStorageImage)
                .setDescriptorCount(1)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        );
    }

    m_pLinkedListDescriptorSetLayout->Finish();


    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice,
            {  m_pDevice->GetDescLayoutPresets().UBO, m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER, m_pLinkedListDescriptorSetLayout }
            , {}
        )
    );
}

void LinkedListGeometryPass::prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    // needn't attachments
    if (m_pDepthAttachmentResource)
    {
        attachments.resize(1);
        attachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
        attachments[0].storeOp = vk::AttachmentStoreOp::eDontCare;
        attachments[0].resourceInitialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        attachments[0].attachmentReferenceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        attachments[0].resource = m_pDepthAttachmentResource->GetNative();
        attachments[0].type = RHI::VulkanFramebuffer::AttachmentType::kDepthStencil;
        attachments[0].resourceFormat = m_pDepthAttachmentResource->GetConfig().format;
    }
}

void LinkedListGeometryPass::prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments)
{
    m_pRenderPass = RHI::VulkanRenderPassBuilder(m_pDevice)
                        .SetAttachments(attachments)
                        .AddSubpassDependency
                        (
                            vk::SubpassDependency()
                                    .setSrcSubpass(0)
                                    .setDstSubpass(VK_SUBPASS_EXTERNAL)
                                    .setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
                                    .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                    .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
                                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                        // )
                        // .AddSubpassDependency
                        // (
                        //     vk::SubpassDependency()
                        //         .setSrcSubpass(0)
                        //         .setDstSubpass(VK_SUBPASS_EXTERNAL)
                        //         .setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
                        //         .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                        //         .setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite)
                        //         .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
                        )
                        .buildUnique();
}

void LinkedListGeometryPass::prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments)
{
    // Geometry frame buffer doesn't need any output attachment.
    m_pFramebuffer = std::make_unique<RHI::VulkanFramebuffer>(m_pDevice, m_pRenderPass.get(), m_fbWidth, m_fbHeight, 1, std::move(attachments));
}

void LinkedListGeometryPass::prepareSBO()
{
    MetaSBOData metaSBOData;
    metaSBOData.count = 0;
    metaSBOData.maxNodeCount = MetaSBOData::MAX_NODE_COUNT * m_fbWidth * m_fbHeight;

    {   // prepare metaBuffer
        m_linkedlistSBOGPUData.metaBuffer.reset(new RHI::VulkanGPUBuffer(
            m_pDevice,
            sizeof(MetaSBOData),
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::SharingMode::eExclusive));
        m_linkedlistSBOGPUData.metaBuffer->FillingBufferOneTime(&metaSBOData, sizeof(metaSBOData));

        {
            // copy to gpu sbo
            vk::Queue queue = m_pDevice->GetVkGraphicQueue();
            vk::CommandBuffer cmd = m_pDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
            m_linkedlistSBOGPUData.metaBuffer->CopyDataToGPU(cmd, queue, sizeof(metaSBOData));
            m_pDevice->GetPVulkanCmdPool()->FreeReUsableCmd(cmd);
        }

        // free staging buffer
        m_linkedlistSBOGPUData.metaBuffer->DestroyCPUBuffer();
    }

    { // prepare indexImage
        RHI::VulkanImageResource::Config resourceConfig;
        resourceConfig.extent = vk::Extent3D(m_fbWidth, m_fbHeight, 1);
        resourceConfig.format = vk::Format::eR32Uint;
        resourceConfig.imageUsage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst;

        m_linkedlistSBOGPUData.headIndexImageResource.reset(new RHI::VulkanImageResource(
            m_pDevice, vk::MemoryPropertyFlagBits::eDeviceLocal, resourceConfig
        ));

        m_linkedlistSBOGPUData.headIndexImageResource->TransitionImageLayout(
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer
        );
    }

    { // prepare linkedListBuffer
        m_linkedlistSBOGPUData.linkedListBuffer.reset(new RHI::VulkanBuffer(
            m_pDevice,
            sizeof(LinkedListNode) * metaSBOData.maxNodeCount,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::SharingMode::eExclusive
        ));
    }
}

void LinkedListGeometryPass::preparePipeline()
{
    auto sampleCount = vk::SampleCountFlagBits::e1;

    {
        auto shaderSet = std::make_shared<RHI::VulkanShaderSet>(m_pDevice);
        shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/linkedlist-geometry.vert.spv", vk::ShaderStageFlagBits::eVertex);
        shaderSet->AddShader(Util::File::getResourcePath() / "Shader/GLSL/SPIR-V/linkedlist-geometry.frag.spv", vk::ShaderStageFlagBits::eFragment);
        auto multiSampleState = std::make_shared<RHI::VulkanMultisampleState>(sampleCount);

        RHI::VulkanDepthStencilState::Config depthTestConfig;
        depthTestConfig.DepthWriteEnable = VK_FALSE;
        auto depthTestState = std::make_shared<RHI::VulkanDepthStencilState>(depthTestConfig);
        auto blendStateAttachment = vk::PipelineColorBlendAttachmentState()
                                        .setColorWriteMask(vk::ColorComponentFlags(0xf))
                                        .setBlendEnable(VK_FALSE);
        auto blendState = std::make_shared<RHI::VulkanColorBlendState>(std::vector<vk::PipelineColorBlendAttachmentState>(1, blendStateAttachment));
        m_pRenderPass->AddGraphicRenderPipeline(
                        "geometry",
                            RHI::VulkanRenderPipelineBuilder(m_pDevice, m_pRenderPass.get())
                                .SetVulkanPipelineLayout(m_pPipelineLayout)
                                .SetVulkanMultisampleState(multiSampleState)
                                .SetVulkanColorBlendState(blendState)
                                .SetshaderSet(shaderSet)
                                .SetVulkanDepthStencilState(depthTestState)
                                .buildUnique()
                        );
    }
}

void LinkedListGeometryPass::prepareOutputDescriptorSets()
{
    m_pDescriptorPool.reset(new RHI::VulkanDescriptorPool(
        m_pDevice,
        std::vector<vk::DescriptorPoolSize>{
            { vk::DescriptorType::eStorageBuffer, 2},
            { vk::DescriptorType::eStorageImage, 1},
        },2));
    m_pDescriptors = m_pDescriptorPool->AllocCustomToUpdatedDescriptorSet(m_pLinkedListDescriptorSetLayout.get());

    std::vector<vk::DescriptorBufferInfo> bufferInfo;
    bufferInfo.resize(2);
    bufferInfo[0]
        .setBuffer(*m_linkedlistSBOGPUData.metaBuffer->GetPVkBuf())
        .setOffset(0)
        .setRange(sizeof(MetaSBOData));
    bufferInfo[1]
        .setBuffer(*m_linkedlistSBOGPUData.linkedListBuffer->GetPVkBuf())
        .setOffset(0)
        .setRange(sizeof(LinkedListNode) * MetaSBOData::MAX_NODE_COUNT * m_fbWidth * m_fbHeight);
    vk::DescriptorImageInfo imgInfo;
    imgInfo.setImageLayout(vk::ImageLayout::eGeneral)
            .setImageView(m_linkedlistSBOGPUData.headIndexImageResource->GetVkImageView());

    std::vector<vk::WriteDescriptorSet> writeDescs;
    writeDescs.resize(3);
    writeDescs[0]
        //.setDstSet( m_vkDescSets[0] )
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setDescriptorCount(1)
        .setBufferInfo(bufferInfo[0])
        ;
    writeDescs[1]
        //.setDstSet( m_vkDescSets[1] )
        .setDstBinding(1)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
        .setDescriptorCount(1)
        .setBufferInfo(bufferInfo[1])
        ;
    writeDescs[2]
        //.setDstSet( m_vkDescSets[2] )
        .setDstBinding(2)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eStorageImage)
        .setDescriptorCount(1)
        .setImageInfo(imgInfo)
        ;
    m_pDescriptors->UpdateDescriptorSets(writeDescs);
}