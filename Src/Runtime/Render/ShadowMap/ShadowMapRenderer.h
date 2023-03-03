#pragma once
#include "Runtime/Render/Camera.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Util/Mathutil.h"
#include "vulkan/vulkan_handles.hpp"
namespace Render {

class ShadowMapRenderer : public RendererBase
{
public:
    explicit ShadowMapRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~ShadowMapRenderer() override;

    std::vector<RHI::Model*> GetModels() override { return {m_pModel.get(), m_pCubeModel.get()}; }
    Camera* GetCamera() override { return m_pCamera.get(); }
    Lights* GetLights() override { return m_pLights.get(); }

protected:
    void prepare() override;
    virtual void prepareModel();
    void prepareRenderpass() override;
    void render() override;

protected:
    void prepareLights();
    void preparePipeline();
    void prepareFrameBuffer();
    void prepareCamera();
    void prepareInputCallback();
    void prepareShadowMapPass();
    void prepareDebugPass();

    void updateShadowMapMVPUniformBuf(uint32_t currentFrameIdx);

protected:
    std::unique_ptr<RHI::Model> m_pModel;
    std::unique_ptr<RHI::Model> m_pCubeModel;
    std::unique_ptr<RHI::Model> m_pDebugShadowMapQuadModel;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLights;
    RHI::ShadowMapRenderPass* m_pShadwomapPass;
    uint32_t m_imageIdx = 0;
    bool m_renderFromLight = false;
};

}