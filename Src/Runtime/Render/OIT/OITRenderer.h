#pragma once
#include "Runtime/Render/OIT/LinkedList/LinkedListColorPass.h"
#include "Runtime/Render/OIT/LinkedList/LinkedListGeometryPass.h"
#include "Runtime/Render/PostPass/PostPass.h"
#include "Runtime/Render/PrePass/GeometryPrePass.h"
#include "Runtime/Render/PrePass/PrePass.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class OITRenderer : public RendererBase
{
public:
    explicit OITRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~OITRenderer() override = default;

protected:
    void prepare() override;
    void render() override;
    // 1. layout
    void prepareLayout() override;
    // 2. attachments
    // void preparePresentFramebufferAttachments() = default;
    // 3. renderpass
    void prepareRenderpass() override;
    // 4. framebuffer = default
    void preparePresentFramebuffer() override;
    // 5. pipeline
    void preparePipeline() override;

    std::vector<RHI::Model*> GetModels() override { return {m_pModel.get()}; }
    Camera* GetCamera() override { return m_pCamera.get(); }
    Lights* GetLights() override { return m_pLight.get(); }
private:
    RHI::VulkanPhysicalDevice::Config enableDeviceAtomicFeature(const RHI::VulkanPhysicalDevice::Config& physicalConfig);

    // prepare camera
    void prepareCamera();
    // prepare Light
    void prepareLight();
    // prepare descriptor layout
    void prepareModel();
    // prepare callback
    void prepareInputCallback();

    void prepareLinkedListPass();
private:
    struct LinkedListPass
    {
        std::unique_ptr<LinkedListGeometryPass> geometryPass;
        std::unique_ptr<LinkedListColorPass> colorPass;
    };

    struct OpaquePass
    {
        std::vector<RHI::VulkanFramebuffer::Attachment> attachments;
        std::unique_ptr<RHI::VulkanFramebuffer> framebuffer;
        std::unique_ptr<RHI::VulkanImageSampler> colorAttachmentSampler;
        std::unique_ptr<RHI::VulkanImageSampler> depthAttachmentSampler;

        std::unique_ptr<RHI::VulkanDescriptorPool> descriptorPool;
        std::shared_ptr<RHI::VulkanDescriptorSets> descriptorSet;
    };

private:
    std::unique_ptr<RHI::Model> m_pSkyBox;
    std::unique_ptr<RHI::Model> m_pModel;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLight;

    LinkedListPass m_linkedlistPass;
    OpaquePass m_opaquePass;
};

}