#pragma once

#include "Runtime/Render/PrePass/PrePass.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include <memory>
namespace Render {

class LinkedListGeometryPass : public PrePass
{
public:
    explicit LinkedListGeometryPass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight, RHI::VulkanImageResource* depthAttachmentResource = nullptr);
    ~LinkedListGeometryPass() override;

    void Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models) override;
    RHI::VulkanDescriptorSets* GetDescriptorSets() const override { return m_pDescriptors.get(); }

    std::shared_ptr<RHI::VulkanDescriptorSetLayout> GetLinkedListDescriptorSetLayout() const { return m_pLinkedListDescriptorSetLayout; }
private:
    void prepareLayout() override;
    void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) override;
    void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) override;
    void preparePipeline() override;
    void prepareOutputDescriptorSets() override;

    void prepareSBO();
private:
    struct MetaSBOData
    {
        static constexpr uint32_t MAX_NODE_COUNT = 20;
        uint32_t count;
        uint32_t maxNodeCount;
    };
    struct LinkedListNode
    {
        glm::vec4 color;
        float depth;
        uint32_t next;
    };

    struct LinkedListSBOGPUData
    {
        std::unique_ptr<RHI::VulkanGPUBuffer> metaBuffer;
        std::unique_ptr<RHI::VulkanImageResource> headIndexImageResource;
        std::unique_ptr<RHI::VulkanBuffer> linkedListBuffer;
    };
private:
    std::shared_ptr<RHI::VulkanDescriptorSetLayout> m_pLinkedListDescriptorSetLayout;
    uint32_t m_fbWidth;
    uint32_t m_fbHeight;
    RHI::VulkanImageResource* m_pDepthAttachmentResource;
    LinkedListSBOGPUData m_linkedlistSBOGPUData;
};

}