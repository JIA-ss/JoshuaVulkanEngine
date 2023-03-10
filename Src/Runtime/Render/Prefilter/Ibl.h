#pragma once
#include <vulkan/vulkan.hpp>
#include "Runtime/Render/Camera.h"
#include "Runtime/Render/Light.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanCommandPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "vulkan/vulkan_handles.hpp"

namespace Render { namespace Prefilter {


class Ibl
{
public:
    struct Framebuffer
    {
        vk::Framebuffer native;
        std::unique_ptr<RHI::VulkanImageResource> attachment;
    };
public:
    explicit Ibl(RHI::VulkanDevice* device) : m_pDevice(device) { }
    void Prepare();
    void FillToBindingDescriptorSets(std::vector<vk::DescriptorSet>& tobinding);

private:
    void prepareLayout();
    void prepareCamera();
    void prepareSamplerCubeModel();
    void prepareCmd();

    void generateIrradianceCubeMap();
    void generatePrefilterEnvCubeMap();
    void generateBrdfLUT();

    void generateOutputDiscriptorSet();

private:
    struct PushConstant
    {
        glm::mat4 mvp = glm::mat4(1.0f);
        float deltaPhi = 0.1f;
        float deltaTheta = 0.1f;
        float roughness = 0.0f;
        float numSamples = 32.0f;
    };

private:
    RHI::VulkanDevice* m_pDevice;


    std::shared_ptr<RHI::VulkanPipelineLayout> m_pPipelineLayout;
    std::unique_ptr<Camera> m_pCamera;
    std::unique_ptr<Lights> m_pLight;
    std::unique_ptr<RHI::Model> m_pSamplerCubeModel;
    std::unique_ptr<RHI::VulkanCommandPool> m_pCmdPool;

    // irradiance
    std::unique_ptr<RHI::VulkanImageSampler> m_pIrradianceCubeMapSampler;
    std::unique_ptr<RHI::VulkanRenderPass> m_pIrradianceRenderPass;
    std::unique_ptr<Framebuffer> m_pIrradianceFramebuffer;

    // prefilterEnvironment
    std::unique_ptr<RHI::VulkanImageSampler> m_pPrefilterEnvCubeMapSampler;
    std::unique_ptr<RHI::VulkanRenderPass> m_pPrefilterEnvRenderPass;
    std::unique_ptr<Framebuffer> m_pPrefilterEnvFramebuffer;

    // brdfLUT
    std::unique_ptr<RHI::VulkanImageSampler> m_pBrdfLUTSampler;
    std::unique_ptr<RHI::VulkanRenderPass> m_pBrdfLUTRenderPass;
    std::unique_ptr<Framebuffer> m_pBrdfLUTFramebuffer;

    std::unique_ptr<RHI::VulkanDescriptorPool> m_pDescPool;
    std::shared_ptr<RHI::VulkanDescriptorSets> m_pDescriptor;
};

}}

