#pragma once
#include "Runtime/Render/Light.h"
#include "Runtime/Render/Prefilter/Ibl.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class PBRRenderer : public RendererBase
{

protected:
    std::unique_ptr<RHI::Model> m_pModel;
    std::unique_ptr<RHI::Model> m_pSkyboxModel;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLight;
    std::unique_ptr<Prefilter::Ibl> m_pIbl;
    uint32_t m_imageIdx = 0;

public:
    explicit PBRRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~PBRRenderer() override;

    std::vector<RHI::Model*> GetModels() override { return {m_pModel.get()}; }
    Camera* GetCamera() override { return m_pCamera.get(); }
    Lights* GetLights() override { return m_pLight.get(); }
protected:
    void prepare() override;
    virtual void prepareModel();
    void prepareRenderpass() override;
    void render() override;

protected:
    void prepareLayout() override;
    void preparePipeline();
    void prepareFrameBuffer();
    void prepareCamera();
    void prepareLight();
    void prepareInputCallback();
    void prepareIbl();
    void updateLightUniformBuf(uint32_t currentFrameIdx);
private:
    struct PushConstant
    {
        glm::vec4 vec4 = glm::vec4(1.0);
    };
    PushConstant m_pushConstant;
};

}