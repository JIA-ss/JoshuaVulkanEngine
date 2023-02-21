#pragma once
#include "Runtime/VulkanRHI/Layout/VulkanPipelineLayout.h"
#include "Runtime/VulkanRHI/VulkanSwapchain.h"
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/SimpleModel/SimpleModelRenderer.h>

namespace Render {

class MultiPipelineRenderer : public SimpleModelRenderer
{
protected:

    std::vector<std::shared_ptr<RHI::VulkanRenderPipeline>> m_Pipelines;
public:
    explicit MultiPipelineRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~MultiPipelineRenderer() override;

protected:
    void prepare() override;
    void render() override;

protected:

    void prepareMultiPipelines();
};

}