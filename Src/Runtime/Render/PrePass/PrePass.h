#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <stdint.h>
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class PrePass
{
public:
    // presets
    static std::unique_ptr<PrePass> CreateGeometryPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight);



public:
    explicit PrePass(RHI::VulkanDevice* device, Camera* cam) : m_pDevice(device), m_pCamera(cam) { }
    virtual ~PrePass() = default;
    virtual void Render(vk::CommandBuffer& cmdBuffer, const std::vector<RHI::Model*>& models) = 0;
    virtual RHI::VulkanDescriptorSets* GetDescriptorSets() const = 0;
protected:
    // 1. layout
    virtual void prepareLayout();
    // 2. attachments
    virtual void prepareAttachments(std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) = 0;
    // 3. renderpass
    virtual void prepareRenderPass(const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments) = 0;
    // 4. framebuffer
    virtual void prepareFramebuffer(std::vector<RHI::VulkanFramebuffer::Attachment>&& attachments) = 0;
    // 5. pipeline
    virtual void preparePipeline() = 0;
    // 6. descriptor sets
    virtual void prepareOutputDescriptorSets() = 0;
protected:
    RHI::VulkanDevice* m_pDevice;
    Camera* m_pCamera;
    std::shared_ptr<RHI::VulkanPipelineLayout> m_pPipelineLayout;
    std::unique_ptr<RHI::VulkanRenderPass> m_pRenderPass;
    std::unique_ptr<RHI::VulkanFramebuffer> m_pFramebuffer;
    std::unique_ptr<RHI::VulkanDescriptorPool> m_pDescriptorPool;
    std::shared_ptr<RHI::VulkanDescriptorSets> m_pDescriptors;
};

}