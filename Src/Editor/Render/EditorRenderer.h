#pragma once

#include "Editor/Editor.h"
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/Render/RendererBase.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"

EDITOR_NAMESPACE_BEGIN

class EditorRenderer : public Render::RendererBase
{
public:
    explicit EditorRenderer(
                Render::RendererBase* runtime_renderer,
                const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~EditorRenderer() override;

    std::vector<RHI::Model*> GetModels() override { return {}; }
    Camera* GetCamera() override { return m_pCamera.get(); }

protected:
    void prepare() override;
    void prepareRenderpass() override;
    void render() override;

    void prepareLayout() override;
    void prepareCamera();
    void prepareLight();
    void prepareModel();
    void prepareInputCallback();

    void preparePipeline();
    void prepareFrameBuffer();


    void updateLightUniformBuf(uint32_t currentFrameIdx);
protected:
    Render::RendererBase* m_runtimeRenderer;

    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Render::Lights> m_pLight;
    std::vector<std::shared_ptr<RHI::ModelView>> m_sceneModels;

    std::unique_ptr<RHI::Model> m_pSceneCameraFrustumModel;
    std::vector<std::unique_ptr<RHI::Model>> m_pSceneLightFrustumModels;

    Camera* m_sceneCamera;
    Render::Lights* m_sceneLights;
};


EDITOR_NAMESPACE_END