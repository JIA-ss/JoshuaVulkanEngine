#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanDescriptorSets.h"
#include "Runtime/VulkanRHI/VulkanRenderPass.h"
#include "Runtime/VulkanRHI/VulkanShaderSet.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class SimpleModelRenderer : public RendererBase
{
protected:
    std::vector<RHI::Vertex> m_vertices;
    std::unique_ptr<RHI::VulkanVertexBuffer> m_pVulkanVertexBuffer;

    std::vector<uint32_t> m_indices;
    std::unique_ptr<RHI::VulkanVertexIndexBuffer> m_pVulkanVertexIndexBuffer;

    std::shared_ptr<RHI::VulkanShaderSet> m_pVulkanShaderSet;
    std::shared_ptr<RHI::VulkanDescriptorSetLayout> m_pVulkanDescriptorSetLayout;
    std::shared_ptr<RHI::VulkanDescriptorSets> m_pVulkanDescriptorSets;
    std::shared_ptr<RHI::VulkanRenderPipeline> m_pVulkanRenderPipeline;

    uint32_t m_imageIdx = 0;
public:
    explicit SimpleModelRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~SimpleModelRenderer() override;

protected:
    void prepare() override;
    void prepareRenderpass() override;
    void render() override;

private:
    void prepareVertexData();
    void prepareShader();
    void preparePipeline();
    void prepareFrameBuffer();

    void updateUniformBuf(uint32_t currentFrameIdx);
};

}