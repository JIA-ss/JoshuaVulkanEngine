#pragma once
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class SimpleModelRenderer : public RendererBase
{

protected:
    std::unique_ptr<RHI::Model> m_pModel;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLight;
    uint32_t m_imageIdx = 0;

public:
    explicit SimpleModelRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~SimpleModelRenderer() override;

    std::vector<RHI::Model*> GetModels() override { return {m_pModel.get()}; }
    Camera* GetCamera() override { return m_pCamera.get(); }
protected:
    void prepare() override;
    virtual void prepareModel();
    void prepareRenderpass() override;
    void render() override;

protected:
    void preparePipeline();
    void prepareFrameBuffer();
    void prepareCamera();
    void prepareLight();
    void prepareInputCallback();

    void updateLightUniformBuf(uint32_t currentFrameIdx);

};

}