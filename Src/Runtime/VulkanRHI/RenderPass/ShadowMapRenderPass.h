#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanFramebuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDescriptorPool.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Util/Mathutil.h"

RHI_NAMESPACE_BEGIN

class VulkanDevice;
class ShadowMapRenderPass
{
public:
    static constexpr const uint32_t SHADOWMAP_DEFAULT_DIM = 2048;
    static constexpr const float DEPTH_BIAS_CONSTANT = 1.25f;
    static constexpr const float DEPTH_BIAS_SLOP = 1.75f;
private:
public:
    ShadowMapRenderPass(VulkanDevice* device, int num = 1, uint32_t width = SHADOWMAP_DEFAULT_DIM, uint32_t height = SHADOWMAP_DEFAULT_DIM);
    ~ShadowMapRenderPass();

    void InitModelShadowDescriptor(Model* model);
    void SetShadowPassLightVPUBO(CameraUniformBufferObject& ubo, int frameId, int lightIdx);
    void FillDepthSamplerToBindedDescriptorSetsVector(std::vector<vk::DescriptorSet>& descList, VulkanPipelineLayout* pipelineLayout, int frameId);
    void Render(vk::CommandBuffer cmd, std::vector<Model*> models, int frameId = 0);

protected:
    void initDepthSampler();
    void initRenderPass();
    void initFramebuffer();
    void initDepthSamplerDescriptor();
    void initPipelines();
    void initUniformBuffer();

protected:
    int m_num = 1;
    uint32_t m_width = SHADOWMAP_DEFAULT_DIM;
    uint32_t m_height = SHADOWMAP_DEFAULT_DIM;
    vk::Format m_vkDepthFormat;

    std::shared_ptr<VulkanPipelineLayout> m_pPipelineLayout;

    VulkanDevice* m_pDevice;
    std::unique_ptr<VulkanDescriptorPool> m_pDescriptorPool;

    /*
        No.Light
            <==> No.RenderPass
                <==> (FRAMES) * No.Framebuffer <==> No.DepthSampler <==> (1)DepthSamplerDescriptorSets <==> (1)DescriptorSet
            <==> (FRAMES) * No.UniformBuffer <==> (1)UniformBufferDescriptorSets <==> (No.UniformBuffer)DescriptorSet
            <==> (FRAMES) * No.Event
    */

    // No.Light <==> No.Renderpass
    // each light corresponds to a renderpass
    std::vector<std::shared_ptr<VulkanRenderPass>> m_pRenderPasses;

    // No.Light <==> (FRAMES) * No.Framebuffer
    // each Light(renderpass) corresponds to a framebuffer
    std::vector<std::unique_ptr<VulkanFramebuffer>> m_pVulkanFramebuffers;


    // No.Framebuffer <==> No.DepthSampler
    // each framebuffer depthAttachment corresponds to a sampler
    std::vector<std::unique_ptr<VulkanImageSampler>>  m_pDepthSamplers;

    // ALL.DepthSampler <==> (1)DepthSamplerDescriptorSets
    // all sampler share one descriptorset
    std::shared_ptr<VulkanDescriptorSets>  m_pDepthSamplerDescriptorSets;

    // No.Light <==> (FRAMES) * No.UniformBuffer
    // each light correponds to an UniformBuffer
    std::vector<std::unique_ptr<VulkanBuffer>> m_uniformBuffers;

        // No.UniformBuffer <==> (1)UBODescriptorSets <==> (No.UniformBuffer)DescriptorSet
        // each UniformBuffer corresponds to a descriptorset
    // obsolete, ubo descriptor will saved in model, it will be generated in InitShadowPassUniforDescriptorSets
    // std::array<std::shared_ptr<VulkanDescriptorSets>,               MAX_FRAMES_IN_FLIGHT> m_pUniformBufferDescriptorSets;
};
RHI_NAMESPACE_END