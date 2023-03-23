#pragma once
#include "Runtime/Render/PrePass/PrePass.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include <memory>
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class DeferredRenderer : public RendererBase
{
public:
    explicit DeferredRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~DeferredRenderer() override;

protected:
    void prepare() override;
    void render() override;

    // 1. layout
    void prepareLayout() override;
    // 2. attachments = default
    // 3. renderpass
    void prepareRenderpass() override;
    // 4. framebuffer = default
    // 5. pipeline
    void preparePipeline() override;


    std::vector<RHI::Model*> GetModels() override { return {m_pSceneModel.get()}; }
    Camera* GetCamera() override { return m_pCamera.get(); }
    Lights* GetLights() override { return m_pLight.get(); }
private:
    void prepareGeometryPrePass();
    void prepareCamera();
    void prepareModel();
    void prepareLight();
    void prepareInputCallback();
private:
    struct PushConstant
    {
        int showDebugTarget = 0;
    };
private:
    PushConstant m_pushConstant;
    std::unique_ptr<PrePass> m_pGeometryPass;

    std::unique_ptr<RHI::Model> m_pSceneModel;
    std::unique_ptr<RHI::Model> m_pPlaneModel;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLight;


    std::shared_ptr<RHI::VulkanDescriptorSetLayout> m_pCustomDescriptorSetLayout;

};

}